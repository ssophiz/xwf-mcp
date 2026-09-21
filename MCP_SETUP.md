# XWF-MCP 연결 안내

`xwf-mcp`는 X-Ways Forensics의 X-Tension DLL과 Python MCP stdio 서버를 연결한다.
현재 제공하는 도구는 사용자가 XWF에서 승인한 선택 항목의 제한된 메타데이터를 읽는
`bridge_status`와 `selected_items`이다. 파일 본문을 자동으로 전송하거나 실행하지 않는다.

## 동작 구조

```text
X-Ways Forensics
  -> xwf-mcp.dll (X-Tension)
  -> 현재 Windows 사용자 전용 Named Pipe
  -> server.py (MCP stdio)
  -> Claude Desktop, Codex, 또는 다른 MCP 클라이언트
```

## 사전 준비

PowerShell에서 저장소를 내려받고 의존성을 설치한다.

```powershell
git clone https://github.com/ssophiz/xwf-mcp.git
cd xwf-mcp
py -3.11 -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
```

X-Ways Forensics x64에 맞는 `build\xwf-mcp.dll`을 사용한다. DLL은 XWF의
X-Tensions/Extensions 메뉴에서 로드한다. 항목을 선택한 뒤 X-Tension 작업을 실행하고,
공유 승인 창에서 승인해야 MCP 서버가 스냅샷을 받는다.

## Claude Desktop (로컬 stdio)

Claude Desktop의 MCP 설정 파일에 다음 항목을 추가한다. Windows 경로의 백슬래시는
JSON에서 두 번 쓴다.

```json
{
  "mcpServers": {
    "xwf": {
      "command": "C:\\Users\\DELL\\source\\xwf-mcp\\.venv\\Scripts\\python.exe",
      "args": ["C:\\Users\\DELL\\source\\xwf-mcp\\server.py"]
    }
  }
}
```

Claude를 재시작한 후 XWF에서 항목을 승인한다. 먼저 `bridge_status`를 호출하고,
그 다음 `selected_items`를 호출한다. 스냅샷은 5분 후 만료된다.

## ChatGPT

ChatGPT 웹은 로컬 stdio MCP 서버에 직접 연결하지 않는다. ChatGPT의 Developer Mode에서
사용하려면 MCP 서버를 원격 HTTPS 엔드포인트로 제공하거나, 사설 호스트에서는 OpenAI
Secure MCP Tunnel을 사용해야 한다.

Secure MCP Tunnel 방식의 개요:

1. OpenAI Platform의 Tunnel 설정에서 `tunnel_id`와 runtime API key를 만든다.
2. XWF와 `server.py`가 실행되는 Windows 호스트에서 `tunnel-client`를 실행한다.
3. tunnel-client가 로컬 stdio 명령으로 다음 서버를 실행하도록 설정한다.

```text
python C:\Users\DELL\source\xwf-mcp\server.py
```

4. ChatGPT의 Settings/Workspace Settings → Apps → Create에서 Developer Mode 앱을 만들고
   Tunnel을 연결한다.
5. 도구를 스캔한 뒤 새 채팅의 도구 메뉴에서 XWF 앱을 선택한다.

ChatGPT의 Developer Mode와 MCP 앱 사용 가능 여부는 계정·워크스페이스 요금제와 관리자
권한에 따라 달라진다. ChatGPT 앱은 도구 정의를 승인 시점에 스냅샷하므로 서버 도구를
변경한 뒤에는 관리자 화면에서 Refresh/재승인이 필요할 수 있다.

## 확인 명령

```powershell
.\.venv\Scripts\python.exe -m unittest discover -s . -p 'test*.py' -v
```

테스트는 MCP stdio 핸드셰이크, 도구 목록 조회, Named Pipe 합성 데이터 왕복을 확인한다.
실제 XWF 사건 파일 분석이 성공했다는 뜻은 아니다.

## 보안 및 증거 취급

- 원본 이미지와 XWF 사건 파일은 읽기 전용으로 유지한다.
- MCP에는 수사관이 승인한 항목의 메타데이터만 전달한다.
- 파일 본문·자격증명·개인정보를 자동 전송하지 않는다.
- 악성 파일은 호스트에서 실행하지 않는다.
- ChatGPT 연결 시에는 민감한 증거를 외부 서비스로 보낼 수 있는지 기관 정책을 먼저 확인한다.
- XWF-MCP는 현재 파일 내용 분석기나 악성코드 판정기가 아니다.

## 현재 한계

현재 도구는 `bridge_status`와 `selected_items`만 제공한다. 파일 트리 검색, 해시 계산,
YARA/PE/ELF 정적 분석, 타임라인 생성은 별도 도구로 추가해야 한다.

참고:

- [XWF-MCP 저장소](https://github.com/ssophiz/xwf-mcp)
- [OpenAI: Developer mode and MCP apps in ChatGPT](https://help.openai.com/en/articles/12584461-developer-mode-and-mcp-apps-in-chatgpt)
- [OpenAI: Secure MCP Tunnel](https://developers.openai.com/api/docs/guides/secure-mcp-tunnels)

## Problem definition and token-efficient MCP workflow

Start each investigation by creating a short in-memory brief. Do not place file
contents, credentials, private keys, or unnecessary personal data in it.

```text
problem_definition(
  objective="find suspicious executables",
  scope="user-approved XWF selection",
  constraints="read-only; do not execute or contact indicators",
  requested_output="compact candidate table with evidence",
  artifact_types="ELF, PE, scripts"
)
```

The tool returns only a `problem_id`; keep that ID in the conversation instead
of repeating the complete brief. Then use this sequence:

```text
problem_definition -> bridge_status -> selected_items(fields="id,name")
-> candidate selection -> selected_items(fields="id,name,size")
```

Use small pages (`limit=25` or less) and retain the returned `capture_id` when
requesting the next page. The current allowed projections are `id`, `name`, and
`size`. This keeps the context small and prevents raw evidence contents from
being sent to the model.

### Client instruction prompt

After enabling the XWF MCP app in Claude or ChatGPT, use:

```text
Use xwf-mcp. First call problem_definition with the objective, scope,
read-only constraints, and compact output format. Then call bridge_status.
If a fresh snapshot exists, call selected_items with fields="id,name" and
small pages. Do not request file contents, execute evidence, or contact any
indicator. Expand to fields="id,name,size" only for shortlisted items.
```

ChatGPT web requires a remote HTTPS MCP endpoint or Secure MCP Tunnel; it cannot
connect directly to this local stdio server. Claude Desktop can use the local
stdio configuration above. The provider-neutral behavior rules are in
[`XWF_MCP_SKILL.md`](XWF_MCP_SKILL.md).
