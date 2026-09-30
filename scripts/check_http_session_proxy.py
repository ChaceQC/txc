"""使用本机 HTTP 代理响应验证显式代理与解压后的正文限额。"""

import gzip
import http.server
import os
import pathlib
import subprocess
import threading


ROOT = pathlib.Path(__file__).resolve().parents[1]
SUFFIX = ".exe" if os.name == "nt" else ""
TOOL_DIR = pathlib.Path(os.environ.get("TXC_TOOL_DIR", ROOT / "tx"))
BODY = bytes.fromhex("00ff80010203")


class ProxyHandler(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def do_GET(self):
        if self.path != "http://example.invalid/compressed":
            self.send_error(404)
            return
        compressed = gzip.compress(BODY)
        self.send_response(200)
        self.send_header("Content-Encoding", "gzip")
        self.send_header("Content-Length", str(len(compressed)))
        self.end_headers()
        self.wfile.write(compressed)

    def log_message(self, format, *args):
        pass


def main():
    server = http.server.ThreadingHTTPServer(("127.0.0.1", 19745), ProxyHandler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        source = ROOT / "tests/network/http_session_proxy.tx"
        subprocess.run([TOOL_DIR / f"txc{SUFFIX}", source], cwd=ROOT, check=True)
        subprocess.run([ROOT / f"tx_build/http_session_proxy{SUFFIX}"], cwd=ROOT,
                       check=True, timeout=15)
    finally:
        server.shutdown()
        server.server_close()
        thread.join()


if __name__ == "__main__":
    main()
