# Local mock endpoint

`mock_c2.py` reproduces only the observed HTTP path for offline testing. It
does not execute PowerShell, create persistence, download files, or make
outbound connections.

Run with the default loopback hostname:

```powershell
python .\mock_c2.py
```

Specify a hostname that belongs to the isolated lab and that the operator is
authorized to bind:

```powershell
python .\mock_c2.py --domain lab-c2.local
```

`--host`와 `--domain`은 같은 옵션이다. 입력한 이름은 이 컴퓨터의 로컬
네트워크 인터페이스로 해석되어야 한다. 외부 도메인을 입력한다고 DNS가
자동으로 이 서버를 가리키지는 않는다.

관찰된 샘플 URL의 기본 포트는 HTTP `80`, HTTPS `443`이지만, 모의 서버의
기본 포트는 권한 충돌을 피하기 위해 `8080`이다. 실습 환경에서 포트를
명시하려면 다음처럼 사용한다.

```powershell
python .\mock_c2.py --domain lab-c2.local --port 8080
```

80/443을 사용하려면 해당 포트가 비어 있고 관리자 권한이 있는지 확인해야
한다. 제3자 도메인이나 샘플의 실제 악성 도메인에는 바인딩하지 않는다.

Test without running the sample:

```powershell
Invoke-WebRequest `
  'http://127.0.0.1:8080/setting.php?banana=LAB&apple=testuser&domado=fruit'
```

The server logs the request and returns the inert text
`Write-Output 'MOCK_C2_RESPONSE'`. Do not point the original malware at an
external or third-party hostname. For a later authorized lab deployment, use
an owned test domain and a segregated network, and keep the endpoint's response
non-executable until the protocol is understood.

## Local browser console

Open `http://127.0.0.1:8080/` in a browser. The page is a read-only dashboard
that refreshes `/api/events` every two seconds. It shows request time, client
address, path, and query parameters. It has no command runner, shell, upload,
or outbound-network feature.

`/api/diagnostics` returns only allowlisted read-only values: hostname, Python
version, process uptime, and free disk space. It does not accept a command or
argument to execute.
