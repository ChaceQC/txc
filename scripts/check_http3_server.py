"""用独立 aioquic 客户端验证 TX HTTP/3 服务端本机往返。"""

import asyncio
from datetime import datetime, timedelta, timezone
import ipaddress
import pathlib
import ssl
import subprocess
import sys
import tempfile
import time

from aioquic.asyncio import QuicConnectionProtocol, connect
from aioquic.h3.connection import H3_ALPN, H3Connection
from aioquic.h3.events import DataReceived, HeadersReceived
from aioquic.quic.configuration import QuicConfiguration
from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.serialization import pkcs12
from cryptography.x509.oid import ExtendedKeyUsageOID, NameOID

from check_tls import make_certificate, make_key


ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.stdout.reconfigure(encoding="utf-8")


def fixtures(directory):
    now = datetime.now(timezone.utc)
    root_key = make_key()
    root_name = x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,
                                              "TX HTTP3 test root")])
    root = make_certificate("TX HTTP3 test root", root_key, root_name,
                            root_key, 101, now, True)
    server_key = make_key()
    server_name = x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,
                                                "127.0.0.1")])
    server = (x509.CertificateBuilder()
        .subject_name(server_name).issuer_name(root.subject)
        .public_key(server_key.public_key()).serial_number(102)
        .not_valid_before(now - timedelta(days=1))
        .not_valid_after(now + timedelta(days=30))
        .add_extension(x509.BasicConstraints(ca=False, path_length=None),
                       critical=True)
        .add_extension(x509.SubjectAlternativeName(
            [x509.IPAddress(ipaddress.ip_address("127.0.0.1"))]),
            critical=False)
        .add_extension(x509.ExtendedKeyUsage([ExtendedKeyUsageOID.SERVER_AUTH]),
                       critical=False)
        .sign(root_key, hashes.SHA256()))
    (directory / "root.der").write_bytes(
        root.public_bytes(serialization.Encoding.DER))
    (directory / "server.p12").write_bytes(
        pkcs12.serialize_key_and_certificates(b"tx-http3-test", server_key,
            server, [root],
            serialization.BestAvailableEncryption(b"test-only-password")))


class Http3Client(QuicConnectionProtocol):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.http = H3Connection(self._quic)
        self.done = None
        self.status = None
        self.body = bytearray()

    async def get(self):
        self.done = asyncio.get_running_loop().create_future()
        stream_id = self._quic.get_next_available_stream_id()
        self.http.send_headers(stream_id, [
            (b":method", b"GET"), (b":scheme", b"https"),
            (b":authority", b"service.example"),
            (b":path", b"/hello")], end_stream=True)
        self.transmit()
        return await asyncio.wait_for(self.done, 8)

    def quic_event_received(self, event):
        for message in self.http.handle_event(event):
            if isinstance(message, HeadersReceived):
                self.status = dict(message.headers).get(b":status")
            elif isinstance(message, DataReceived):
                self.body.extend(message.data)
                if message.stream_ended and self.done and not self.done.done():
                    self.done.set_result((self.status, bytes(self.body)))


async def request():
    config = QuicConfiguration(is_client=True, alpn_protocols=H3_ALPN)
    config.verify_mode = ssl.CERT_NONE  # 本机独立客户端只检查协议互操作。
    async with connect("127.0.0.1", 19751, configuration=config,
                       create_protocol=Http3Client) as protocol:
        result = await protocol.get()
        if result != (b"200", b"/hello"):
            raise AssertionError(result)


def main():
    with tempfile.TemporaryDirectory(prefix="tx-http3-") as temporary:
        directory = pathlib.Path(temporary)
        fixtures(directory)
        for name in ("http3_server", "http3_client",
                     "http3_untrusted_server", "http3_untrusted_client",
                     "http3_binary_server", "http3_binary_client"):
            source = ROOT / "tests/network" / f"{name}.tx"
            subprocess.run([ROOT / "tx/txc.exe", source], cwd=ROOT,
                           check=True)
        for client_name in ("aioquic", "tx", "binary", "untrusted"):
            server_name = ("http3_untrusted_server" if client_name == "untrusted"
                else "http3_binary_server" if client_name == "binary"
                else "http3_server")
            server = subprocess.Popen(
                [ROOT / "tx_build" / f"{server_name}.exe", directory],
                cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                creationflags=subprocess.CREATE_NO_WINDOW)
            try:
                time.sleep(0.5)
                if server.poll() is not None:
                    output, _ = server.communicate()
                    raise AssertionError(output.decode("utf-8", "replace"))
                if client_name == "aioquic":
                    asyncio.run(request())
                else:
                    program = ("http3_untrusted_client" if client_name == "untrusted"
                        else "http3_binary_client" if client_name == "binary"
                        else "http3_client")
                    result = subprocess.run(
                        [ROOT / "tx_build" / f"{program}.exe", directory],
                        cwd=ROOT, capture_output=True, timeout=15)
                    if result.returncode != 0:
                        raise AssertionError(f"{program} exit={result.returncode}: " +
                            (result.stdout + result.stderr).decode("utf-8", "replace"))
                output, _ = server.communicate(timeout=12)
                if server.returncode != 0:
                    raise AssertionError(output.decode("utf-8", "replace"))
                print("HTTP/3 未知根证书拒绝通过" if client_name == "untrusted"
                      else f"HTTP/3 {client_name} 客户端往返通过")
            finally:
                if server.poll() is None:
                    server.kill()
                    server.wait()


if __name__ == "__main__":
    main()
