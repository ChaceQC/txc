"""验证 Requests 自定义 CA 的发送前校验及客户端身份路径。"""

from __future__ import annotations

from datetime import datetime, timedelta, timezone
import http.server
import ipaddress
import os
from pathlib import Path
import select
import socket
import socketserver
import ssl
import subprocess
import tempfile
import threading
import time

from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.primitives.serialization import pkcs12
from cryptography.x509.oid import ExtendedKeyUsageOID, NameOID


ROOT = Path(__file__).resolve().parents[1]


def key():
    return rsa.generate_private_key(public_exponent=65537, key_size=2048)


def certificate(name, public_key, issuer, signing_key, serial, is_ca,
                purpose=None):
    now = datetime.now(timezone.utc)
    subject = x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, name)])
    value = (x509.CertificateBuilder()
        .subject_name(subject)
        .issuer_name(issuer)
        .public_key(public_key)
        .serial_number(serial)
        .not_valid_before(now - timedelta(days=1))
        .not_valid_after(now + timedelta(days=7))
        .add_extension(x509.BasicConstraints(ca=is_ca,
            path_length=1 if is_ca else None), critical=True)
        .add_extension(x509.KeyUsage(digital_signature=True,
            content_commitment=False, key_encipherment=not is_ca,
            data_encipherment=False, key_agreement=False,
            key_cert_sign=is_ca, crl_sign=is_ca,
            encipher_only=False, decipher_only=False), critical=True))
    if purpose is ExtendedKeyUsageOID.SERVER_AUTH or name == "127.0.0.1":
        value = value.add_extension(x509.SubjectAlternativeName(
            [x509.IPAddress(ipaddress.ip_address("127.0.0.1"))]),
            critical=False)
    if purpose:
        value = value.add_extension(x509.ExtendedKeyUsage([purpose]),
                                    critical=False)
    return value.sign(signing_key, hashes.SHA256())


def fixtures(directory: Path) -> None:
    root_key = key()
    root_name = x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,
        "TX Requests test root")])
    root = certificate("TX Requests test root", root_key.public_key(),
                       root_name, root_key, 1, True)
    unrelated_key = key()
    unrelated_name = x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,
        "TX unrelated test root")])
    unrelated = certificate("TX unrelated test root",
        unrelated_key.public_key(), unrelated_name, unrelated_key, 2, True)
    server_key = key()
    server = certificate("127.0.0.1", server_key.public_key(),
        root.subject, root_key, 3, False, ExtendedKeyUsageOID.SERVER_AUTH)
    wrong_usage = certificate("127.0.0.1", server_key.public_key(),
        root.subject, root_key, 5, False, ExtendedKeyUsageOID.CLIENT_AUTH)
    client_key = key()
    client = certificate("TX Requests client", client_key.public_key(),
        root.subject, root_key, 4, False, ExtendedKeyUsageOID.CLIENT_AUTH)

    (directory / "root.pem").write_bytes(root.public_bytes(
        serialization.Encoding.PEM))
    (directory / "other.pem").write_bytes(unrelated.public_bytes(
        serialization.Encoding.PEM))
    (directory / "server.pem").write_bytes(server.public_bytes(
        serialization.Encoding.PEM))
    (directory / "server-key.pem").write_bytes(server_key.private_bytes(
        serialization.Encoding.PEM, serialization.PrivateFormat.PKCS8,
        serialization.NoEncryption()))
    (directory / "wrong-usage.pem").write_bytes(wrong_usage.public_bytes(
        serialization.Encoding.PEM))
    (directory / "client.p12").write_bytes(
        pkcs12.serialize_key_and_certificates(b"tx-client", client_key,
            client, [root], serialization.BestAvailableEncryption(
                b"test-only-password")))
    (directory / "client-empty.p12").write_bytes(
        pkcs12.serialize_key_and_certificates(b"tx-client-empty", client_key,
            client, [root], serialization.NoEncryption()))
    (directory / "client-long.p12").write_bytes(
        pkcs12.serialize_key_and_certificates(b"tx-client-long", client_key,
            client, [root], serialization.BestAvailableEncryption(b"x" * 4096)))


class Handler(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"
    requests_seen = 0
    authorization_seen = []
    bodies_seen = []
    client_ports = []
    retry_seen = 0

    def _record(self) -> None:
        type(self).requests_seen += 1
        type(self).authorization_seen.append(self.headers.get("Authorization", ""))
        type(self).client_ports.append(self.client_address[1])
        length = int(self.headers.get("Content-Length", "0"))
        type(self).bodies_seen.append(self.rfile.read(length))

    def _respond(self) -> None:
        self._record()
        body = b"custom-ca-ok"
        self.send_response(200)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self) -> None:
        if self.path.endswith("/redirect"):
            self._record()
            self.send_response(302)
            self.send_header("Location", "/secure")
            self.send_header("Content-Length", "0")
            self.end_headers()
            return
        if self.path.endswith("/retry"):
            type(self).retry_seen += 1
            if type(self).retry_seen == 1:
                self._record()
                self.close_connection = True
                try:
                    self.request.shutdown(socket.SHUT_RDWR)
                except OSError:
                    pass
                self.request.close()
                return
        self._respond()

    def do_POST(self) -> None:
        self._respond()

    def log_message(self, format: str, *args: object) -> None:
        pass


class ConnectProxyHandler(socketserver.BaseRequestHandler):
    def handle(self) -> None:
        reader = self.request.makefile("rb")
        request_line = reader.readline().decode("ascii").strip().split()
        if len(request_line) != 3 or request_line[0] != "CONNECT":
            self.request.sendall(b"HTTP/1.1 405 Method Not Allowed\r\n\r\n")
            return
        for line in reader:
            if line in (b"\r\n", b"\n", b""):
                break
        host, port_text = request_line[1].rsplit(":", 1)
        upstream = socket.create_connection((host, int(port_text)), timeout=5)
        try:
            self.request.sendall(
                b"HTTP/1.1 200 Connection Established\r\n\r\n")
            sockets = (self.request, upstream)
            while True:
                readable, _, _ = select.select(sockets, (), (), 10)
                if not readable:
                    continue
                for source in readable:
                    data = source.recv(16384)
                    if not data:
                        return
                    destination = upstream if source is self.request else self.request
                    destination.sendall(data)
        finally:
            upstream.close()


def main() -> None:
    with tempfile.TemporaryDirectory(prefix="tx-requests-tls-") as location:
        directory = Path(location)
        fixtures(directory)
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        context.load_cert_chain(directory / "server.pem",
                                directory / "server-key.pem")
        context.load_verify_locations(cafile=directory / "root.pem")
        context.verify_mode = ssl.CERT_REQUIRED
        server = http.server.ThreadingHTTPServer(("0.0.0.0", 0), Handler)
        server.socket = context.wrap_socket(server.socket, server_side=True)
        worker = threading.Thread(target=server.serve_forever, daemon=True)
        worker.start()
        proxy = socketserver.ThreadingTCPServer(
            ("127.0.0.1", 0), ConnectProxyHandler)
        proxy.daemon_threads = True
        proxy_worker = threading.Thread(target=proxy.serve_forever, daemon=True)
        proxy_worker.start()
        wrong_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        wrong_context.load_cert_chain(directory / "wrong-usage.pem",
                                      directory / "server-key.pem")
        wrong_context.load_verify_locations(cafile=directory / "root.pem")
        wrong_context.verify_mode = ssl.CERT_REQUIRED
        wrong_server = http.server.ThreadingHTTPServer(("0.0.0.0", 0), Handler)
        wrong_server.socket = wrong_context.wrap_socket(
            wrong_server.socket, server_side=True)
        wrong_worker = threading.Thread(
            target=wrong_server.serve_forever, daemon=True)
        wrong_worker.start()
        try:
            program = directory / "requests-tls.exe"
            subprocess.run([str(ROOT / "tx/txc.exe"),
                str(ROOT / "tests/network/requests_11_8_tls.tx"),
                "-o", str(program)], cwd=ROOT, check=True, timeout=90)
            environment = os.environ.copy()
            environment["PATH"] = str(ROOT / "tx") + os.pathsep + environment["PATH"]
            try:
                result = subprocess.run([str(program),
                    f"https://127.0.0.1:{server.server_port}/secure",
                    str(directory / "root.pem"), str(directory / "other.pem"),
                    str(directory / "client.p12"),
                    f"https://127.0.0.2:{server.server_port}/secure",
                    str(directory / "client-empty.p12"),
                    str(directory / "client-long.p12"), "x" * 4096,
                    "x" * 4097,
                    f"http://127.0.0.1:{proxy.server_address[1]}",
                    f"https://127.0.0.1:{wrong_server.server_port}/secure"],
                    cwd=ROOT, env=environment,
                    capture_output=True, timeout=30)
            except subprocess.TimeoutExpired as failure:
                output = (failure.stdout or b"") + (failure.stderr or b"")
                raise AssertionError(
                    "Requests TLS 用例超时；阶段输出：" +
                    output.decode("utf-8", "replace")) from failure
            if result.returncode:
                output = (result.stdout + result.stderr).decode("utf-8", "replace")
                raise AssertionError(
                    f"Requests TLS 用例退出码 {result.returncode}: {output}")
            if Handler.requests_seen != 7:
                raise AssertionError(
                    f"有效 CA 请求及验证失败路径共收到 {Handler.requests_seen} 个 HTTP 请求")
            if Handler.authorization_seen != ["Bearer audit-only"] * 6 + [""] or \
                    Handler.bodies_seen != [b"synthetic-body"] * 6 + [b""]:
                raise AssertionError("服务端收到的认证头或正文不符合合成验收值")
            if Handler.client_ports[0] != Handler.client_ports[1]:
                raise AssertionError("同一 Requests 会话没有复用已验证的 TLS 连接")
            if Handler.retry_seen != 2:
                raise AssertionError("自定义 CA 请求没有按原契约重试 GET")

            client_program = directory / "requests-h2-client.exe"
            subprocess.run([str(ROOT / "tx/txc.exe"),
                str(ROOT / "tests/network/requests_11_8_h2_client.tx"),
                "-o", str(client_program)], cwd=ROOT, check=True, timeout=90)
            server_program = directory / "requests-h2-server.exe"
            subprocess.run([str(ROOT / "tx/txc.exe"),
                str(ROOT / "tests/network/requests_11_8_h2_server.tx"),
                "-o", str(server_program)], cwd=ROOT, check=True, timeout=90)
            with socket.socket() as probe:
                probe.bind(("127.0.0.1", 0))
                port = probe.getsockname()[1]
            h2_server = subprocess.Popen([str(server_program), str(port),
                str(directory / "server.pem"),
                str(directory / "server-key.pem")], cwd=ROOT, env=environment,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            try:
                time.sleep(0.25)
                if h2_server.poll() is not None:
                    output = h2_server.communicate()
                    raise AssertionError(
                        "HTTP/2 TLS 测试服务端提前退出：" +
                        (output[0] + output[1]).decode("utf-8", "replace"))
                client_result = subprocess.run([str(client_program),
                    f"https://127.0.0.1:{port}/secure",
                    str(directory / "root.pem")], cwd=ROOT, env=environment,
                    capture_output=True, timeout=20)
                if client_result.returncode:
                    output = (client_result.stdout + client_result.stderr).decode(
                        "utf-8", "replace")
                    raise AssertionError(
                        f"HTTP/2 TLS 客户端退出码 {client_result.returncode}: {output}")
                server_out, server_err = h2_server.communicate(timeout=10)
                if h2_server.returncode:
                    output = (server_out + server_err).decode("utf-8", "replace")
                    raise AssertionError(
                        f"HTTP/2 TLS 服务端退出码 {h2_server.returncode}: {output}")
            finally:
                if h2_server.poll() is None:
                    h2_server.terminate()
                    h2_server.wait(timeout=5)
        finally:
            proxy.shutdown()
            proxy.server_close()
            proxy_worker.join()
            wrong_server.shutdown()
            wrong_server.server_close()
            wrong_worker.join()
            server.shutdown()
            server.server_close()
            worker.join()
    print("REQUESTS_TLS_SEQ2_OK")


if __name__ == "__main__":
    main()
