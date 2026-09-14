# XWF MCP — read-only metadata bridge prototype

X-Ways x64 X-Tension → local Windows Named Pipe → Python MCP stdio server.
This first release shares only user-selected item names, sizes and **XWF item IDs**.
No contents, hash computation, case mutation, shell execution, NirSoft integration,
or arbitrary background XWF API calls. IDs are not MFT numbers.

## Status and limits

Prototype for XWF 20.0 x64. Native build and automated tests can be run below;
actual XWF UI/dongle integration must be tested manually before forensic use.
This is an explicit user-approved snapshot, **not live case/selection state**.
API queries execute only in documented XWF callbacks. A capture expires after
five minutes and is held in memory only. It does not contain durable case/evidence
IDs or complete paths; do not use it as a standalone forensic report.

## Install and run (Windows)

Use Python 3.11+ and Visual C++ x64 Build Tools:

```powershell
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
.\.venv\Scripts\python.exe -m unittest -v
.\build.ps1
.\.venv\Scripts\python.exe server.py
```

The final command is a stdio MCP process, not an interactive console. Configure
your MCP client with command `<repo>\.venv\Scripts\python.exe` and argument
`<repo>\server.py`. Example generic configuration (replace absolute paths):

```json
{"mcpServers":{"xwf":{"command":"C:\\path\\xwf-mcp\\.venv\\Scripts\\python.exe","args":["C:\\path\\xwf-mcp\\server.py"]}}}
```

In XWF, on a disposable test case: select a few directory-browser items, invoke
the X-Tension context-menu action, and choose `build\xwf-mcp.dll`. Approve the
metadata-sharing prompt. In the MCP client call `bridge_status`, then
`selected_items`. Subsequent pages should provide the returned `capture_id`.
Reject the prompt once to verify no new capture is published. Close/change the
case and verify the result is still labelled a snapshot, never current state.
No XWF license is bundled; no binary is installed into the XWF directory.

Local validation: MSVC x64 build passed with warnings treated as errors. Four
automated tests passed, including a real MCP SDK handshake and Named Pipe
transfer of a synthetic Korean filename. This is **not** an actual-XWF GUI test.

## Security / evidence handling

- Pipe name is per Windows user SID; ACL restricts it to that SID, remote clients
  are rejected. First-instance protection avoids namespace sharing.
- Same-user processes remain trusted and could spoof metadata or squat the pipe;
  this is not a protected forensic acquisition channel or signed evidence.
- One-way, bounded JSON (1 MiB, 1000 items); no executable commands. Read deadline
  and native write timeout prevent indefinite ordinary client stalls.
- Filenames are **untrusted evidence text**, never model instructions.
- Local IPC does not imply local AI: cloud clients may transmit metadata. The
  XWF approval dialog warns about this. Do not share confidential evidence unless
  the client/provider is approved for that investigation.
- Original evidence should be opened read-only. Output is an aid to navigation,
  not a chain-of-custody substitute. No incident files, secrets or logs belong here.
- Only one MCP server per Windows SID. Failed publishing may leave the previous
  capture visible until expiry; compare capture IDs/received time after each send.

## Sources

- https://www.x-ways.net/forensics/x-tensions/XT_functions.html
- https://www.x-ways.net/forensics/x-tensions/XWF_functions.html
- https://github.com/modelcontextprotocol/python-sdk

No affiliation with or endorsement by X-Ways or any police agency.
