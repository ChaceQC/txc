"""以临时双向 TLS 服务验证 Requests 自定义 CA 与客户端 PKCS#12。"""

from __future__ import annotations

from datetime import datetime, timedelta, timezone
import http.server
import ipaddress
import os
from pathlib import Path
import ssl
import subprocess
import tempfile
import threading

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
    if purpose is ExtendedKeyUsageOID.SERVER_AUTH:
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
    (directory / "client.p12").write_bytes(
        pkcs12.serialize_key_and_certificates(b"tx-client", client_key,
            client, [root], serialization.BestAvailableEncryption(
                b"test-only-password")))


class Handler(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def do_GET(self) -> None:
        body = b"mutual-tls-ok"
        self.send_response(200)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, format: str, *args: object) -> None:
        pass


def main() -> None:
    with tempfile.TemporaryDirectory(prefix="tx-requests-tls-") as location:
        directory = Path(location)
        fixtures(directory)
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        context.load_cert_chain(directory / "server.pem",
                                directory / "server-key.pem")
        context.load_verify_locations(cafile=directory / "root.pem")
        context.verify_mode = ssl.CERT_REQUIRED
        server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        server.socket = context.wrap_socket(server.socket, server_side=True)
        worker = threading.Thread(target=server.serve_forever, daemon=True)
        worker.start()
        try:
            program = directory / "requests-tls.exe"
            subprocess.run([str(ROOT / "tx/txc.exe"),
                str(ROOT / "tests/network/requests_11_8_tls.tx"),
                "-o", str(program)], cwd=ROOT, check=True, timeout=90)
            environment = os.environ.copy()
            environment["PATH"] = str(ROOT / "tx") + os.pathsep + environment["PATH"]
            result = subprocess.run([str(program),
                f"https://127.0.0.1:{server.server_port}/secure",
                str(directory / "root.pem"), str(directory / "other.pem"),
                str(directory / "client.p12"),
                f"https://localhost:{server.server_port}/secure"],
                cwd=ROOT, env=environment,
                capture_output=True, timeout=30)
            if result.returncode:
                output = (result.stdout + result.stderr).decode("utf-8", "replace")
                raise AssertionError(
                    f"Requests TLS 用例退出码 {result.returncode}: {output}")
        finally:
            server.shutdown()
            server.server_close()
            worker.join()
    print("REQUESTS_11_8_TLS_OK")


if __name__ == "__main__":
    main()
