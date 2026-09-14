// x64 X-Tension; metadata APIs are called exclusively in XWF callbacks.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <sddl.h>
#include <objbase.h>
#include <string>
#include <vector>

using GetName = LPWSTR (__stdcall *)(DWORD);
using GetSize = INT64 (__stdcall *)(LONG);
static GetName getName;
static GetSize getSize;
static HWND parentWindow;
static DWORD version;
static bool approved = false, failed = false;
static std::vector<std::string> items;
static size_t pendingBytes = 0;

static std::string quote(const wchar_t* text) {
    std::string out = "\"";
    const char* hex = "0123456789abcdef";
    for (size_t i = 0; text && text[i]; ++i) {
        if (i >= 32768) throw 1;
        unsigned c = text[i];
        out += "\\u";
        out += hex[(c >> 12) & 15]; out += hex[(c >> 8) & 15];
        out += hex[(c >> 4) & 15]; out += hex[c & 15];
    }
    return out + "\"";
}

static bool publish(const std::string& payload) {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return false;
    DWORD needed = 0;
    GetTokenInformation(token, TokenUser, nullptr, 0, &needed);
    std::vector<BYTE> buffer(needed);
    bool ok = GetTokenInformation(token, TokenUser, buffer.data(), needed, &needed) != 0;
    CloseHandle(token);
    if (!ok) return false;
    LPWSTR sid = nullptr;
    if (!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(buffer.data())->User.Sid, &sid)) return false;
    std::wstring path = L"\\\\.\\pipe\\xwf-mcp-";
    path += sid; LocalFree(sid);
    HANDLE pipe = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED | SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION, nullptr);
    if (pipe == INVALID_HANDLE_VALUE) return false;
    OVERLAPPED ov{}; ov.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!ov.hEvent) { CloseHandle(pipe); return false; }
    DWORD written = 0;
    ok = WriteFile(pipe, payload.data(), static_cast<DWORD>(payload.size()), &written, &ov) != 0;
    if (!ok && GetLastError() == ERROR_IO_PENDING) {
        if (WaitForSingleObject(ov.hEvent, 1000) == WAIT_OBJECT_0)
            ok = GetOverlappedResult(pipe, &ov, &written, FALSE) != 0;
        else {
            CancelIoEx(pipe, &ov);
            GetOverlappedResult(pipe, &ov, &written, TRUE);
        }
    }
    ok = ok && written == payload.size();
    if (ok) {
        ResetEvent(ov.hEvent);
        char ack = 0; DWORD read = 0;
        ok = ReadFile(pipe, &ack, 1, &read, &ov) != 0;
        if (!ok && GetLastError() == ERROR_IO_PENDING) {
            if (WaitForSingleObject(ov.hEvent, 1000) == WAIT_OBJECT_0)
                ok = GetOverlappedResult(pipe, &ov, &read, FALSE) != 0;
            else {
                CancelIoEx(pipe, &ov);
                GetOverlappedResult(pipe, &ov, &read, TRUE);
            }
        }
        ok = ok && read == 1 && ack == '1';
    }
    CloseHandle(ov.hEvent); CloseHandle(pipe);
    return ok;
}

extern "C" __declspec(dllexport) LONG __stdcall XT_Init(DWORD ver, DWORD, HANDLE hwnd, void*) {
    version = ver; parentWindow = static_cast<HWND>(hwnd);
    HMODULE host = GetModuleHandleW(nullptr);
    getName = reinterpret_cast<GetName>(GetProcAddress(host, "XWF_GetItemName"));
    getSize = reinterpret_cast<GetSize>(GetProcAddress(host, "XWF_GetItemSize"));
    return getName && getSize ? 0x05 : -1; // non-thread-safe, understands op flags
}

extern "C" __declspec(dllexport) LONG __stdcall XT_Prepare(HANDLE volume, HANDLE, DWORD op, void*) {
    items.clear(); approved = false; failed = false; pendingBytes = 0;
    if (!volume || (op & 255) != 4) return -1; // selection context menu only
    approved = MessageBoxW(parentWindow,
        L"Share selected file names, sizes and XWF item IDs with the local MCP client?\n"
        L"A cloud AI client may transmit these values outside this computer.\n"
        L"No file contents or hashes will be read. Maximum 1000 items.",
        L"XWF MCP - approve metadata sharing", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDYES;
    return approved ? 0x31 : -1; // selected items, directories, zero-byte items
}

extern "C" __declspec(dllexport) LONG __stdcall XT_ProcessItem(LONG id, void*) {
    if (!approved || failed) return 0;
    try {
        if (items.size() >= 1000 || id < 0) throw 1;
        auto name = getName(static_cast<DWORD>(id));
        if (!name) throw 1;
        items.push_back("{\"id\":" + std::to_string(id) + ",\"name\":" + quote(name) +
            ",\"size\":" + std::to_string(getSize(id)) + "}");
        pendingBytes += items.back().size() + 1;
        if (pendingBytes > 1024 * 1024 - 512) throw 1;
    } catch (...) { failed = true; }
    return 0;
}

extern "C" __declspec(dllexport) LONG __stdcall XT_Finalize(HANDLE, HANDLE, DWORD, void*) {
    if (!approved) return 0;
    approved = false;
    try {
        if (failed) throw 1;
        GUID guid{}; if (FAILED(CoCreateGuid(&guid))) throw 1;
        wchar_t text[40]{}; if (!StringFromGUID2(guid, text, 40)) throw 1;
        std::string payload = "{\"schema\":1,\"capture_id\":" + quote(text) +
            ",\"source_pid\":" + std::to_string(GetCurrentProcessId()) +
            ",\"version\":" + std::to_string(version) + ",\"items\":[";
        for (size_t i = 0; i < items.size(); ++i) {
            if (i) payload += ',';
            payload += items[i];
            if (payload.size() > 1024 * 1024 - 2) throw 1;
        }
        payload += "]}";
        if (!publish(payload)) throw 1;
        MessageBoxW(parentWindow, L"Metadata sent. Ask the MCP client to check the received capture.",
            L"XWF MCP", MB_OK);
    } catch (...) {
        MessageBoxW(parentWindow, L"Nothing confirmed. Start the MCP server, or reduce selection size.\n"
            L"A previous capture may remain until its five-minute expiry.", L"XWF MCP", MB_OK | MB_ICONERROR);
    }
    items.clear(); return 0;
}

extern "C" __declspec(dllexport) LONG __stdcall XT_Done(void*) {
    items.clear(); approved = false; return 0;
}
