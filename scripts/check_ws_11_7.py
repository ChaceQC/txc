"""验证 WebSocket 的共享 Upgrade、WSS、关闭原因和任务式收发。"""

from pathlib import Path
import base64
import hashlib
import os
import socket
import ssl
import struct
import subprocess
import sys
import tempfile
import threading
import time

from check_tls import fixtures


ROOT = Path(__file__).resolve().parents[1]


def free_port():
    with socket.socket() as selected:
        selected.bind(("127.0.0.1", 0))
        return selected.getsockname()[1]


def environment():
    values = os.environ.copy()
    values["PATH"] = str(ROOT / "tx") + os.pathsep + values["PATH"]
    return values


def compile_case(source, destination, env):
    result = subprocess.run(
        [str(ROOT / "tx/txc.exe"), str(source), "-o", str(destination)],
        cwd=ROOT, env=env, capture_output=True, timeout=90)
    if result.returncode:
        output = (result.stdout + result.stderr).decode("utf-8", "replace")
        raise AssertionError(f"WebSocket 程序编译失败：{output}")


def connect_retry(port):
    deadline = time.monotonic() + 5
    while time.monotonic() < deadline:
        try:
            connection = socket.create_connection(("127.0.0.1", port), 1)
            connection.settimeout(5)
            return connection
        except OSError:
            time.sleep(0.05)
    raise AssertionError("WebSocket 测试服务没有启动")


def read_exact(connection, length):
    result = bytearray()
    while len(result) < length:
        block = connection.recv(length - len(result))
        if not block:
            raise AssertionError("WebSocket 连接提前关闭")
        result.extend(block)
    return bytes(result)


def read_head(connection):
    result = bytearray()
    while not result.endswith(b"\r\n\r\n"):
        result.extend(read_exact(connection, 1))
        if len(result) > 65536:
            raise AssertionError("HTTP 响应头过长")
    return bytes(result)


def upgrade(connection):
    key = base64.b64encode(b"0123456789abcdef").decode("ascii")
    request = (
        "GET /ws HTTP/1.1\r\nHost: service.example\r\n"
        "Connection: Upgrade\r\nUpgrade: websocket\r\n"
        f"Sec-WebSocket-Key: {key}\r\n"
        "Sec-WebSocket-Version: 13\r\n\r\n"
    )
    connection.sendall(request.encode("ascii"))
    response = read_head(connection)
    if not response.startswith(b"HTTP/1.1 101 "):
        raise AssertionError(f"WebSocket Upgrade 失败：{response!r}")


def send_frame(connection, opcode, payload=b""):
    if len(payload) > 125:
        raise AssertionError("测试控制帧过长")
    mask = b"\x12\x34\x56\x78"
    encoded = bytes(byte ^ mask[index % 4]
                    for index, byte in enumerate(payload))
    connection.sendall(bytes((0x80 | opcode, 0x80 | len(payload))) +
                       mask + encoded)


def read_frame(connection):
    first, second = read_exact(connection, 2)
    if second & 0x80:
        raise AssertionError("服务端帧不能带掩码")
    length = second & 0x7f
    if length == 126:
        length = struct.unpack("!H", read_exact(connection, 2))[0]
    elif length == 127:
        length = struct.unpack("!Q", read_exact(connection, 8))[0]
    return first & 0x0f, read_exact(connection, length)


def run_server(program, arguments, client):
    server = subprocess.Popen(
        [str(program), *(str(item) for item in arguments)], cwd=ROOT,
        env=environment(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    try:
        client()
        output, error = server.communicate(timeout=12)
    finally:
        if server.poll() is None:
            server.kill()
            server.communicate()
    if server.returncode:
        details = (output + error).decode("utf-8", "replace")
        raise AssertionError(f"WebSocket 服务端退出码 {server.returncode}：{details}")


def check_upgrade(program):
    port = free_port()

    def client():
        with connect_retry(port) as ordinary:
            ordinary.sendall(b"GET /health HTTP/1.1\r\nHost: localhost\r\n\r\n")
            head = read_head(ordinary)
            if not head.startswith(b"HTTP/1.1 200 "):
                raise AssertionError("共享监听器的普通 HTTP 请求失败")
            if read_exact(ordinary, 7) != b"healthy":
                raise AssertionError("普通 HTTP 正文错误")
        with connect_retry(port) as peer:
            upgrade(peer)
            if read_frame(peer) != (9, b"\x01\x02"):
                raise AssertionError("服务端 ping 错误")
            if read_frame(peer) != (10, b""):
                raise AssertionError("服务端 pong 错误")
            send_frame(peer, 10, b"\x01\x02")
            send_frame(peer, 1, b"")
            if read_frame(peer) != (1, b"ok"):
                raise AssertionError("合法空消息与关闭状态混淆")
            send_frame(peer, 8, struct.pack("!H", 1001) + b"bye")
            if read_frame(peer) != (8, struct.pack("!H", 1001) + b"bye"):
                raise AssertionError("WebSocket 关闭握手错误")

    run_server(program, [port], client)


def check_tls(program, directory):
    port = free_port()
    root = (directory / "root.der").read_bytes()
    context = ssl.create_default_context(cadata=ssl.DER_cert_to_PEM_cert(root))

    def client():
        with connect_retry(port) as raw:
            with context.wrap_socket(raw, server_hostname="service.example") as peer:
                peer.settimeout(5)
                upgrade(peer)
                send_frame(peer, 1, b"secure")
                if read_frame(peer) != (1, b"secure-ok"):
                    raise AssertionError("WSS 文本往返错误")
                if read_frame(peer) != (8, struct.pack("!H", 1000) + b"done"):
                    raise AssertionError("WSS 关闭原因错误")
                send_frame(peer, 8, struct.pack("!H", 1000))

    run_server(program, [directory, port], client)


def check_async(program):
    def client():
        with connect_retry(19761) as peer:
            upgrade(peer)
            send_frame(peer, 1, b"async")
            if read_frame(peer) != (1, b"async-ok"):
                raise AssertionError("异步任务收发错误")
            if read_frame(peer) != (8, struct.pack("!H", 1000) + b"done"):
                raise AssertionError("异步任务关闭原因错误")
            send_frame(peer, 8, struct.pack("!H", 1000))

    run_server(program, [], client)


def check_client_close(program):
    with socket.socket() as listener:
        listener.bind(("127.0.0.1", 0))
        listener.listen(1)
        listener.settimeout(5)
        port = listener.getsockname()[1]
        errors = []

        def serve():
            try:
                with listener.accept()[0] as peer:
                    peer.settimeout(5)
                    request = read_head(peer)
                    headers = {}
                    for line in request.split(b"\r\n")[1:]:
                        if b":" in line:
                            name, value = line.split(b":", 1)
                            headers[name.lower()] = value.strip()
                    key = headers[b"sec-websocket-key"]
                    accept = base64.b64encode(hashlib.sha1(key +
                        b"258EAFA5-E914-47DA-95CA-C5AB0DC85B11").digest())
                    peer.sendall(b"HTTP/1.1 101 Switching Protocols\r\n"
                        b"Upgrade: websocket\r\nConnection: Upgrade\r\n"
                        b"Sec-WebSocket-Accept: " + accept + b"\r\n\r\n")
                    payload = struct.pack("!H", 1001) + b"server"
                    peer.sendall(bytes((0x88, len(payload))) + payload)
                    time.sleep(0.2)
            except Exception as error:
                errors.append(error)

        worker = threading.Thread(target=serve)
        worker.start()
        result = subprocess.run([str(program), str(port)], cwd=ROOT,
            env=environment(), capture_output=True, timeout=10)
        worker.join(timeout=6)
        if worker.is_alive() or errors or result.returncode:
            output = (result.stdout + result.stderr).decode("utf-8", "replace")
            raise AssertionError(f"TX WebSocket 客户端关闭状态失败：{errors} {output}")


def main():
    env = environment()
    with tempfile.TemporaryDirectory(prefix="tx-ws-11-7-") as location:
        directory = Path(location)
        fixtures(directory)
        upgrade_program = directory / "upgrade.exe"
        tls_program = directory / "tls.exe"
        async_program = directory / "async.exe"
        close_client_program = directory / "close-client.exe"
        compile_case(ROOT / "tests/network/ws_upgrade_server.tx",
                     upgrade_program, env)
        compile_case(ROOT / "tests/network/ws_tls_server.tx", tls_program, env)
        compile_case(ROOT / "tests/network/ws_async_server.tx",
                     async_program, env)
        compile_case(ROOT / "tests/network/ws_close_client.tx",
                     close_client_program, env)
        check_upgrade(upgrade_program)
        check_tls(tls_program, directory)
        check_async(async_program)
        check_client_close(close_client_program)
    print("WS_11_7_OK")


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    main()
