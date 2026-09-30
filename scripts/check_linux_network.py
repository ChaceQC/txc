"""定向检查 Linux 网络端口：DNS/异步 socket、HTTP 会话与 WebSocket 客户端。"""

from __future__ import annotations

import base64
import gzip
import hashlib
import http.server
import os
from pathlib import Path
import socket
import struct
import subprocess
import tempfile
import threading
import time

ROOT = Path(__file__).resolve().parents[1]
TOOL_DIR = Path(os.environ.get("TXC_TOOL_DIR", ROOT / "tx"))
SUFFIX = ".exe" if os.name == "nt" else ""


def compile_case(name: str, directory: Path) -> Path:
    target = directory / (name + SUFFIX)
    subprocess.run([str(TOOL_DIR / ("txc" + SUFFIX)),
        str(ROOT / "tests/network" / (name + ".tx")), "-o", str(target)],
        cwd=ROOT, check=True, timeout=90)
    return target


def run(program: Path, *args: object) -> None:
    subprocess.run([str(program), *map(str, args)], cwd=ROOT,
                   check=True, timeout=20)


class Handler(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def answer(self, data: bytes, **headers: str) -> None:
        self.send_response(200)
        self.send_header("Content-Length", str(len(data)))
        for name, value in headers.items():
            self.send_header(name, value)
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self) -> None:
        if self.path == "/id":
            self.answer(str(self.client_address[1]).encode())
        elif self.path == "/set":
            self.answer(b"set", **{"Set-Cookie": "session=linux; Path=/"})
        elif self.path == "/cookies":
            self.answer(self.headers.get("Cookie", "").encode())
        elif self.path == "/gzip":
            self.answer(gzip.compress(b"123456"), **{"Content-Encoding": "gzip"})
        elif self.path == "/slow":
            self.send_response(200)
            self.send_header("Content-Length", "4")
            self.end_headers()
            self.wfile.flush()
            time.sleep(0.35)
            try:
                self.wfile.write(b"slow")
            except (BrokenPipeError, ConnectionResetError):
                pass
        else:
            self.send_error(404)

    def do_POST(self) -> None:
        data = self.rfile.read(int(self.headers["Content-Length"]))
        self.send_response(200)
        self.send_header("Transfer-Encoding", "chunked")
        self.end_headers()
        for offset in range(0, len(data), 3071):
            chunk = data[offset:offset + 3071]
            self.wfile.write(f"{len(chunk):x}\r\n".encode() + chunk + b"\r\n")
        self.wfile.write(b"0\r\n\r\n")

    def log_message(self, *_: object) -> None:
        pass


def exact(peer: socket.socket, size: int) -> bytes:
    result = b""
    while len(result) < size:
        data = peer.recv(size - len(result))
        if not data:
            raise AssertionError("WebSocket 对端提前关闭")
        result += data
    return result


def frame(peer: socket.socket) -> tuple[int, bytes]:
    flags, length = exact(peer, 2)
    if not length & 0x80:
        raise AssertionError("Linux WebSocket 客户端没有掩码")
    length &= 0x7f
    if length == 126:
        length = struct.unpack("!H", exact(peer, 2))[0]
    elif length == 127:
        length = struct.unpack("!Q", exact(peer, 8))[0]
    if length > 65536:
        raise AssertionError("测试 WebSocket 帧过大")
    mask = exact(peer, 4)
    payload = exact(peer, length)
    return flags & 15, bytes(value ^ mask[index % 4]
        for index, value in enumerate(payload))


def websocket(program: Path) -> None:
    errors = []
    with socket.socket() as listener:
        listener.bind(("127.0.0.1", 0))
        listener.listen(1)
        listener.settimeout(10)

        def serve() -> None:
            try:
                with listener.accept()[0] as peer:
                    peer.settimeout(5)
                    request = b""
                    while not request.endswith(b"\r\n\r\n"):
                        request += exact(peer, 1)
                    headers = dict(line.split(b": ", 1)
                        for line in request.split(b"\r\n")[1:] if b": " in line)
                    accept = base64.b64encode(hashlib.sha1(headers[b"Sec-WebSocket-Key"] +
                        b"258EAFA5-E914-47DA-95CA-C5AB0DC85B11").digest())
                    peer.sendall(b"HTTP/1.1 101 Switching Protocols\r\n"
                        b"Upgrade: websocket\r\nConnection: Upgrade\r\n"
                        b"Sec-WebSocket-Accept: " + accept + b"\r\n\r\n")
                    assert frame(peer) == (1, "你好".encode())
                    # 让 UTF-8 文本跨帧，并在中间插入 ping 控制帧。
                    peer.sendall(b"\x01\x03" + "你".encode() + b"\x89\x01p" +
                                 b"\x80\x04" + "好!".encode())
                    assert frame(peer) == (10, b"p")
                    assert frame(peer) == (2, bytes.fromhex("00ff80"))
                    peer.sendall(b"\x82\x03" + bytes.fromhex("00ff80"))
                    opcode, data = frame(peer)
                    assert opcode == 8
                    peer.sendall(bytes((0x88, len(data))) + data)
            except BaseException as error:
                errors.append(error)

        worker = threading.Thread(target=serve, daemon=True)
        worker.start()
        run(program, listener.getsockname()[1])
        worker.join(6)
        if worker.is_alive() or errors:
            raise AssertionError(f"WebSocket 回环验证失败: {errors}")


def main() -> None:
    with tempfile.TemporaryDirectory(prefix="tx-linux-network-") as directory:
        directory = Path(directory)
        for name in ("dns", "socket_behavior"):
            run(compile_case(name, directory))
        http_program = compile_case("http_session_linux", directory)
        ws = compile_case("ws_linux_client", directory)
        server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        worker = threading.Thread(target=server.serve_forever, daemon=True)
        worker.start()
        try:
            run(http_program, server.server_port)
        finally:
            server.shutdown()
            server.server_close()
            worker.join()
        websocket(ws)
    print("LINUX_NETWORK_OK")


if __name__ == "__main__":
    main()
