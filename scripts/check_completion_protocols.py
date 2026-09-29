"""固定种子的 HTTP/2 前言及 PostgreSQL TLS 协议畸形输入验证。"""

from pathlib import Path
import random
import socket
import ssl
import struct
import tempfile
import threading
import sys

from check_completion_chains import run
from check_tls_stream import compile_case, environment
from check_requests_11_8_tls import fixtures
from check_http_protocol_limits import compile_server, run_case

ROOT = Path(__file__).resolve().parent.parent


def malformed_postgres(listener, context, payload, failures):
    try:
        listener.settimeout(5)
        peer, _ = listener.accept()
        with peer:
            peer.settimeout(5)
            request = bytearray()
            while len(request) < 8:
                block = peer.recv(8 - len(request))
                if not block:
                    raise AssertionError("未收到 SSLRequest")
                request.extend(block)
            assert request == struct.pack("!II", 8, 80877103)
            peer.sendall(b"S")
            with context.wrap_socket(peer, server_side=True) as secure:
                assert secure.recv(4096)
                secure.sendall(payload)
    except Exception as error:
        failures.append(type(error).__name__)
    finally:
        listener.close()


def main():
    rng = random.Random(20260930)
    if "postgres" not in sys.argv[1:]:
        compile_server("http2_invalid_server")
        for _ in range(8):
            preface = bytearray(b"PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n")
            preface[rng.randrange(len(preface))] ^= 1 << rng.randrange(8)
            run_case("http2_invalid_server", 19750, bytes(preface))
        print("通过：8 个固定种子 HTTP/2 前言变异")
    env = environment()
    with tempfile.TemporaryDirectory(prefix="tx-pg-protocol-") as location:
        directory = Path(location)
        fixtures(directory)
        program = directory / "postgres_protocol.exe"
        compile_case(ROOT / "tests/completion/postgres_protocol.tx", program, env)
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        context.load_cert_chain(directory / "server.pem", directory / "server-key.pem")
        for index in range(8):
            listener = socket.socket()
            listener.bind(("127.0.0.1", 0))
            port = listener.getsockname()[1]
            listener.listen()
            payload = bytes([ord("R") if index % 2 else 0xff]) + struct.pack("!I", rng.choice([0, 3, 0x7fffffff]))
            payload += bytes(rng.randrange(256) for _ in range(16))
            failures = []
            worker = threading.Thread(target=malformed_postgres, args=(listener, context, payload, failures), daemon=True)
            worker.start()
            try:
                run([program, directory / "root.pem", str(port)], directory, env)
            finally:
                worker.join(6)
            assert not failures and not worker.is_alive(), failures
    print("通过：8 个 TLS 验证后 PostgreSQL 畸形认证/长度帧，客户端有界失败")


if __name__ == "__main__":
    main()
