"""第 11.9 项：本机慢连接、跳转、解压、取消和大文件流检查。"""

from __future__ import annotations

import gzip
import hashlib
import http.server
import os
from pathlib import Path
import socket
import ssl
import subprocess
import sys
import tempfile
import threading
import time

from check_requests_11_8_tls import fixtures


ROOT = Path(__file__).resolve().parents[1]
SOURCES = ROOT / "tests/network"
PATTERN = bytes.fromhex("00ff80") * (4 * 1024 * 1024)
EXPECTED_DIGEST = hashlib.sha256(PATTERN).digest()
sys.stdout.reconfigure(encoding="utf-8")


def environment() -> dict[str, str]:
    values = os.environ.copy()
    values["PATH"] = str(ROOT / "tx") + os.pathsep + values["PATH"]
    return values


def compile_case(name: str, directory: Path) -> Path:
    target = directory / f"{name}.exe"
    result = subprocess.run([str(ROOT / "tx/txc.exe"),
        str(SOURCES / f"{name}.tx"), "-o", str(target)],
        cwd=ROOT, env=environment(), capture_output=True, timeout=90)
    if result.returncode:
        raise AssertionError(result.stdout.decode("utf-8", "replace") +
                             result.stderr.decode("utf-8", "replace"))
    return target


def run_case(program: Path, *args: str) -> None:
    result = subprocess.run([str(program), *args], cwd=ROOT,
        env=environment(), capture_output=True, timeout=35)
    if result.returncode:
        raise AssertionError(f"{program.name} 返回 {result.returncode}: " +
            (result.stdout + result.stderr).decode("utf-8", "replace"))


def start_server(handler: type[http.server.BaseHTTPRequestHandler],
                 context: ssl.SSLContext | None = None):
    server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), handler)
    if context:
        server.socket = context.wrap_socket(server.socket, server_side=True)
    worker = threading.Thread(target=server.serve_forever, daemon=True)
    worker.start()
    return server, worker


def stop_server(server, worker) -> None:
    server.shutdown()
    server.server_close()
    worker.join()


class QuietHandler(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def answer(self, body: bytes, *, encoding: str = "") -> None:
        self.send_response(200)
        self.send_header("Content-Length", str(len(body)))
        if encoding:
            self.send_header("Content-Encoding", encoding)
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, format: str, *args: object) -> None:
        pass


def redirect_handlers(target: list[str]):
    class Origin(QuietHandler):
        def do_GET(self) -> None:
            if self.path in ("/same", "/cross", "/downgrade"):
                destination = "/echo" if self.path == "/same" else target[0]
                self.send_response(302)
                self.send_header("Location", destination)
                self.send_header("Content-Length", "0")
                self.end_headers()
                return
            if self.path == "/echo":
                self.answer(echo_presence(self))

    class Target(QuietHandler):
        def do_GET(self) -> None:
            self.answer(echo_presence(self))

    return Origin, Target


def echo_presence(request: QuietHandler) -> bytes:
    auth = int(bool(request.headers.get("Authorization")))
    cookie = int(bool(request.headers.get("Cookie")))
    return f"auth={auth};cookie={cookie}".encode("ascii")


class Compressed(QuietHandler):
    def do_GET(self) -> None:
        self.answer(gzip.compress(b"A" * (1024 * 1024)), encoding="gzip")


class LargeFile(QuietHandler):
    upload_ok = False

    def do_GET(self) -> None:
        self.send_response(200)
        self.send_header("Content-Length", str(len(PATTERN)))
        self.end_headers()
        for offset in range(0, len(PATTERN), 16384):
            self.wfile.write(PATTERN[offset:offset + 16384])

    def do_POST(self) -> None:
        remaining = int(self.headers.get("Content-Length", "0"))
        digest = hashlib.sha256()
        while remaining:
            chunk = self.rfile.read(min(16384, remaining))
            if not chunk:
                break
            digest.update(chunk)
            remaining -= len(chunk)
        LargeFile.upload_ok = remaining == 0 and digest.digest() == EXPECTED_DIGEST
        self.answer(b"ok" if LargeFile.upload_ok else b"bad")


def free_port() -> int:
    with socket.socket() as selected:
        selected.bind(("127.0.0.1", 0))
        return selected.getsockname()[1]


def check_slow(program: Path) -> None:
    port = free_port()
    process = subprocess.Popen([str(program), str(port)], cwd=ROOT,
        env=environment(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    try:
        time.sleep(0.25)
        with socket.create_connection(("127.0.0.1", port), timeout=2) as slow:
            for value in b"GET /slow":
                try:
                    slow.sendall(bytes([value]))
                except OSError:
                    break
                time.sleep(0.14)
        with socket.create_connection(("127.0.0.1", port), timeout=2) as peer:
            peer.settimeout(2)
            peer.sendall(b"GET / HTTP/1.1\r\nHost: local\r\n\r\n")
            response = bytearray()
            while True:
                chunk = peer.recv(1024)
                if not chunk:
                    break
                response.extend(chunk)
            if b"\r\n\r\nok" not in response:
                raise AssertionError("慢连接后监听器未能正常响应")
        output, error = process.communicate(timeout=4)
        if process.returncode:
            raise AssertionError((output + error).decode("utf-8", "replace") +
                                 f"退出码 {process.returncode}")
    finally:
        if process.poll() is None:
            process.kill()
            process.communicate()


def check_redirect(program: Path, directory: Path) -> None:
    target_url = [""]
    origin_handler, target_handler = redirect_handlers(target_url)
    target, target_worker = start_server(target_handler)
    origin, origin_worker = start_server(origin_handler)
    fixtures(directory)
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain(directory / "server.pem", directory / "server-key.pem")
    secure, secure_worker = start_server(origin_handler, context)
    try:
        target_url[0] = f"http://localhost:{target.server_port}/echo"
        run_case(program, f"http://127.0.0.1:{origin.server_port}",
                 f"https://127.0.0.1:{secure.server_port}",
                 str(directory / "root.pem"))
    finally:
        for item in ((origin, origin_worker), (secure, secure_worker),
                     (target, target_worker)):
            stop_server(*item)


def check_large(program: Path, directory: Path) -> None:
    LargeFile.upload_ok = False
    server, worker = start_server(LargeFile)
    try:
        source = directory / "large-source.bin"
        source.write_bytes(PATTERN)
        run_case(program, f"http://127.0.0.1:{server.server_port}",
                 str(source), str(directory / "large-target.bin"),
                 str(directory / "large-reply.bin"))
        if not LargeFile.upload_ok:
            raise AssertionError("12 MiB 上传摘要不一致")
    finally:
        stop_server(server, worker)


def main() -> None:
    with tempfile.TemporaryDirectory(prefix="tx-network-11-9-") as location:
        directory = Path(location)
        programs = {name: compile_case(f"network_11_9_{name}", directory)
                    for name in ("slow_server", "redirect", "compression",
                                 "cancel", "large_file")}
        check_slow(programs["slow_server"])
        print("11.9 慢连接总时限与后续请求通过")
        check_redirect(programs["redirect"], directory)
        print("11.9 同源、跨源和 HTTPS 降级凭据检查通过")
        server, worker = start_server(Compressed)
        try:
            run_case(programs["compression"],
                     f"http://127.0.0.1:{server.server_port}")
        finally:
            stop_server(server, worker)
        print("11.9 压缩比例与解压正文上限通过")
        run_case(programs["cancel"])
        print("11.9 活动取消后句柄状态通过")
        check_large(programs["large_file"], directory)
        print("11.9 12 MiB 下载与上传通过")


if __name__ == "__main__":
    main()
