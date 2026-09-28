"""对 Requests 11.8 的会话、分块、代理、Cookie 和重试做本机定向验证。"""

from __future__ import annotations

import http.server
import os
from pathlib import Path
import socket
import subprocess
import tempfile
import threading


ROOT = Path(__file__).resolve().parents[1]
COUNTS: dict[str, int] = {}
LOCK = threading.Lock()


def count(path: str) -> int:
    with LOCK:
        COUNTS[path] = COUNTS.get(path, 0) + 1
        return COUNTS[path]


class MainHandler(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def answer(self, status: int, body: bytes, cookies: tuple[str, ...] = ()) -> None:
        self.send_response(status)
        self.send_header("Content-Length", str(len(body)))
        for cookie in cookies:
            self.send_header("Set-Cookie", cookie)
        self.end_headers()
        self.wfile.write(body)

    def drop(self) -> None:
        self.close_connection = True
        self.connection.shutdown(socket.SHUT_RDWR)
        self.connection.close()

    def do_GET(self) -> None:
        attempt = count(self.path)
        if self.path == "/id":
            self.answer(200, str(self.client_address[1]).encode())
        elif self.path == "/stream":
            self.answer(200, b"a" * 50000)
        elif self.path == "/set":
            self.answer(200, b"ok", (
                "sid=base; Path=/; Max-Age=3600",
                "old=bad; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT",
                "__Secure-bad=x; Path=/; Secure",
                "path_only=v; Path=/private; Max-Age=3600",
                "priority=yes; Path=/; Max-Age=3600; "
                "Expires=Thu, 01 Jan 1970 00:00:00 GMT",
                "future=ok; Path=/; Expires=Wed, 21 Oct 2037 07:28:00 GMT",
                "__Host-invalid=x; Secure",
                "foreign=x; Domain=example.invalid; Path=/",
            ))
        elif self.path == "/delete":
            self.answer(200, b"ok", ("sid=gone; Path=/; Max-Age=0",))
        elif self.path.endswith("/echo"):
            self.answer(200, self.headers.get("Cookie", "").encode())
        elif self.path == "/retry":
            if attempt == 1:
                self.drop()
            else:
                self.answer(200, b"retried")
        elif self.path == "/status":
            self.answer(503, b"unavailable")
        else:
            self.answer(404, b"not found")

    def do_POST(self) -> None:
        count(self.path)
        size = int(self.headers.get("Content-Length", "0"))
        self.rfile.read(size)
        if self.path == "/post_fail":
            self.drop()
        elif self.path == "/upload":
            self.answer(200, b"upload-ok")
        else:
            self.answer(404, b"not found")

    def log_message(self, format: str, *args: object) -> None:
        pass


class ProxyHandler(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def do_GET(self) -> None:
        count("proxy")
        if self.path != "http://example.invalid/through":
            self.send_error(404)
            return
        self.send_response(200)
        self.send_header("Content-Length", "8")
        self.end_headers()
        self.wfile.write(b"proxy-ok")

    def log_message(self, format: str, *args: object) -> None:
        pass


def start(handler: type[http.server.BaseHTTPRequestHandler]):
    server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), handler)
    worker = threading.Thread(target=server.serve_forever, daemon=True)
    worker.start()
    return server, worker


def run(program: Path, arguments: list[str]) -> None:
    environment = os.environ.copy()
    environment["PATH"] = str(ROOT / "tx") + os.pathsep + environment["PATH"]
    result = subprocess.run([str(program), *arguments], cwd=ROOT,
                            env=environment, capture_output=True, timeout=30)
    if result.returncode:
        output = (result.stdout + result.stderr).decode("utf-8", "replace")
        raise AssertionError(f"Requests HTTP 用例退出码 {result.returncode}: {output}")


def main() -> None:
    main_server, main_worker = start(MainHandler)
    proxy_server, proxy_worker = start(ProxyHandler)
    try:
        with tempfile.TemporaryDirectory(prefix="tx-requests-11-8-") as directory:
            program = Path(directory) / "requests-http.exe"
            subprocess.run([str(ROOT / "tx/txc.exe"),
                str(ROOT / "tests/network/requests_11_8_http.tx"),
                "-o", str(program)], cwd=ROOT, check=True, timeout=90)
            run(program, [f"http://127.0.0.1:{main_server.server_port}",
                f"http://127.0.0.1:{proxy_server.server_port}",
                str(Path(directory) / "download.bin"),
                str(Path(directory) / "source.bin"),
                str(Path(directory) / "response.bin")])
        if COUNTS.get("/status") != 1 or COUNTS.get("/post_fail") != 1:
            raise AssertionError(f"非幂等或状态码请求被重试: {COUNTS}")
        if COUNTS.get("/retry", 0) < 2 or COUNTS.get("proxy") != 1:
            raise AssertionError(f"连接失败重试或代理未生效: {COUNTS}")
    finally:
        for server, worker in ((main_server, main_worker),
                               (proxy_server, proxy_worker)):
            server.shutdown()
            server.server_close()
            worker.join()
    print("REQUESTS_11_8_HTTP_OK")


if __name__ == "__main__":
    main()
