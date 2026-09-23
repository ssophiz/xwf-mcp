"""Local-only mock endpoint for safe, static C2 protocol testing.

This server never executes request data or returned PowerShell. Use only on
systems and hostnames you control. The default bind address is loopback.
"""
import argparse
import json
import threading
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, urlparse

EVENTS = []
EVENTS_LOCK = threading.Lock()

INDEX = """<!doctype html><meta charset="utf-8">
<title>Mock C2 Console</title>
<style>body{font:14px monospace;background:#111;color:#ddd;margin:2rem}
pre{white-space:pre-wrap;background:#1c1c1c;padding:1rem;border-radius:8px}
h1{color:#7dd3fc}</style>
<h1>Local Mock C2 Console</h1>
<p>Read-only request log. No command execution.</p>
<pre id="log">loading...</pre>
<script>
async function refresh(){
 const r=await fetch('/api/events');
 document.querySelector('#log').textContent=JSON.stringify(await r.json(),null,2);
}
refresh(); setInterval(refresh,2000);
</script>"""


class Handler(BaseHTTPRequestHandler):
    response_text = "Write-Output 'MOCK_C2_RESPONSE'"

    def do_GET(self):
        parsed = urlparse(self.path)
        if parsed.path == "/":
            self._send(200, INDEX, "text/html; charset=utf-8")
            return
        if parsed.path == "/api/events":
            with EVENTS_LOCK:
                body = json.dumps(EVENTS[-100:], ensure_ascii=False)
            self._send(200, body, "application/json; charset=utf-8")
            return
        if parsed.path == "/healthz":
            self._send(200, "ok\n", "text/plain; charset=utf-8")
            return
        if parsed.path != "/setting.php":
            self._send(404, "not found\n", "text/plain; charset=utf-8")
            return

        event = {
            "time_utc": datetime.now(timezone.utc).isoformat(),
            "client": self.client_address[0],
            "path": parsed.path,
            "query": parse_qs(parsed.query, keep_blank_values=True),
        }
        with EVENTS_LOCK:
            EVENTS.append(event)
        print(json.dumps(event, ensure_ascii=False), flush=True)
        self._send(200, self.response_text + "\n", "text/plain; charset=utf-8")

    def _send(self, status, body, content_type):
        data = body.encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def log_message(self, *_):
        pass


def main():
    parser = argparse.ArgumentParser(description="Safe local mock C2 HTTP endpoint")
    parser.add_argument(
        "--host",
        default="127.0.0.1",
        help="bind hostname/address; default is loopback (127.0.0.1)",
    )
    parser.add_argument("--port", type=int, default=8080)
    args = parser.parse_args()
    if not 1 <= args.port <= 65535:
        parser.error("port must be between 1 and 65535")

    Handler.response_text = "Write-Output 'MOCK_C2_RESPONSE'"
    server = ThreadingHTTPServer((args.host, args.port), Handler)
    print(f"mock endpoint listening on http://{args.host}:{args.port}/setting.php")
    print("safe mode: no command execution, no outbound requests")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("stopping")
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
