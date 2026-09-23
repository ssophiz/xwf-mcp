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
python .\mock_c2.py --host 127.0.0.1
```

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
