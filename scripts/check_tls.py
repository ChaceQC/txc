"""生成临时 TLS 身份，运行 9.6 的 TX 配置与验证定向用例。"""

from datetime import datetime, timedelta, timezone
from pathlib import Path
import subprocess
import sys
import tempfile

from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.primitives.serialization import pkcs12
from cryptography.x509.oid import ExtendedKeyUsageOID, NameOID
from check_platform import TXC, environment


root = Path(__file__).resolve().parents[1]
sys.stdout.reconfigure(encoding="utf-8")


def make_key():
    return rsa.generate_private_key(public_exponent=65537, key_size=2048)


def make_certificate(name, key, issuer, issuer_key, serial, now, ca,
                     purpose=None):
    subject = x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, name)])
    builder = (x509.CertificateBuilder()
        .subject_name(subject)
        .issuer_name(issuer)
        .public_key(key.public_key())
        .serial_number(serial)
        .not_valid_before(now - timedelta(days=1))
        .not_valid_after(now + timedelta(days=30))
        .add_extension(x509.BasicConstraints(ca=ca, path_length=1 if ca else None),
                       critical=True)
        .add_extension(x509.KeyUsage(digital_signature=True,
            content_commitment=False, key_encipherment=not ca,
            data_encipherment=False, key_agreement=False, key_cert_sign=ca,
            crl_sign=ca, encipher_only=False, decipher_only=False),
            critical=True))
    if purpose is ExtendedKeyUsageOID.SERVER_AUTH:
        builder = builder.add_extension(x509.SubjectAlternativeName(
            [x509.DNSName("service.example")]), critical=False)
    if purpose is not None:
        builder = builder.add_extension(x509.ExtendedKeyUsage([purpose]),
                                        critical=False)
    return builder.sign(issuer_key, hashes.SHA256())


def fixtures(directory):
    now = datetime.now(timezone.utc)
    root_key = make_key()
    root_name = x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,
        "TX TLS test root")])
    root_cert = make_certificate("TX TLS test root", root_key, root_name,
                                 root_key, 1, now, True)
    other_key = make_key()
    other_name = x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,
        "TX unrelated root")])
    other_cert = make_certificate("TX unrelated root", other_key, other_name,
                                  other_key, 2, now, True)
    server_key = make_key()
    server_cert = make_certificate("service.example", server_key,
        root_cert.subject, root_key, 3, now, False,
        ExtendedKeyUsageOID.SERVER_AUTH)
    client_key = make_key()
    client_cert = make_certificate("TX client", client_key,
        root_cert.subject, root_key, 4, now, False,
        ExtendedKeyUsageOID.CLIENT_AUTH)
    for name, certificate in (("root", root_cert),
                              ("other_root", other_cert),
                              ("server", server_cert),
                              ("client", client_cert)):
        (directory / f"{name}.der").write_bytes(certificate.public_bytes(
            serialization.Encoding.DER))
    for name, key, certificate in (("server", server_key, server_cert),
                                   ("client", client_key, client_cert)):
        package = pkcs12.serialize_key_and_certificates(
            b"tx-tls-test", key, certificate, [root_cert],
            serialization.BestAvailableEncryption(b"test-only-password"))
        (directory / f"{name}.p12").write_bytes(package)


def run(*command):
    values = environment()
    result = subprocess.run([str(item) for item in command], cwd=root,
                            env=values, capture_output=True, timeout=90)
    output = (result.stdout + result.stderr).decode("utf-8", errors="replace")
    if result.returncode:
        raise AssertionError(f"exit={result.returncode}\n{output}")
    return output.strip()


def main():
    with tempfile.TemporaryDirectory(prefix="tx-tls-") as location:
        directory = Path(location)
        fixtures(directory)
        program = directory / "tls_check.exe"
        run(TXC, root / "tests/crypto/tls.tx", "-o", program)
        print(run(program, directory))
        example = directory / "tls_example.exe"
        run(TXC, root / "examples/tls_config.tx", "-o", example)
        print(run(example, directory / "server.der", directory / "root.der"))


if __name__ == "__main__":
    main()
