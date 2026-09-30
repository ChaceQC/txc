"""为 X.509 9.5 生成临时证书与 PKCS#12，运行一次 TX 定向验证。"""

from datetime import datetime, timedelta, timezone
from pathlib import Path
import os
import subprocess
import sys
import tempfile

from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.primitives.serialization import pkcs12
from cryptography.x509.oid import ExtendedKeyUsageOID, NameOID
from check_platform import TOOL_DIR, TXC, environment


root = Path(__file__).resolve().parents[1]
sys.stdout.reconfigure(encoding="utf-8")


def key():
    return rsa.generate_private_key(public_exponent=65537, key_size=2048)


def certificate(subject, issuer, public_key, signer, serial, now,
                start, end, ca, server=False, signing=False):
    builder = (x509.CertificateBuilder()
        .subject_name(x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, subject)]))
        .issuer_name(issuer)
        .public_key(public_key)
        .serial_number(serial)
        .not_valid_before(now + start)
        .not_valid_after(now + end)
        .add_extension(x509.BasicConstraints(ca=ca, path_length=1 if ca else None),
                       critical=True)
        .add_extension(x509.KeyUsage(digital_signature=True, content_commitment=False,
            key_encipherment=not ca, data_encipherment=False, key_agreement=False,
            key_cert_sign=ca, crl_sign=ca, encipher_only=False, decipher_only=False),
            critical=True))
    if server:
        builder = (builder.add_extension(x509.SubjectAlternativeName(
            [x509.DNSName("service.example")]), critical=False)
            .add_extension(x509.ExtendedKeyUsage([ExtendedKeyUsageOID.SERVER_AUTH]),
                           critical=False))
    elif signing:
        builder = builder.add_extension(x509.ExtendedKeyUsage(
            [ExtendedKeyUsageOID.CODE_SIGNING]), critical=False)
    return builder.sign(signer, hashes.SHA256())


def fixtures(directory):
    now = datetime.now(timezone.utc)
    root_key, middle_key, leaf_key = key(), key(), key()
    root_cert = certificate("TX test root", x509.Name([
        x509.NameAttribute(NameOID.COMMON_NAME, "TX test root")]),
        root_key.public_key(), root_key, 100, now,
        -timedelta(days=1), timedelta(days=365), True)
    middle_cert = certificate("TX test intermediate", root_cert.subject,
        middle_key.public_key(), root_key, 101, now,
        -timedelta(days=1), timedelta(days=180), True)
    leaf_cert = certificate("service.example", middle_cert.subject,
        leaf_key.public_key(), middle_key, 102, now,
        -timedelta(days=1), timedelta(days=30), False, True)
    expired_cert = certificate("service.example", middle_cert.subject,
        leaf_key.public_key(), middle_key, 103, now,
        -timedelta(days=60), -timedelta(days=30), False, True)
    future_cert = certificate("service.example", middle_cert.subject,
        leaf_key.public_key(), middle_key, 104, now,
        timedelta(days=2), timedelta(days=30), False, True)
    non_ca_key = key()
    non_ca_cert = certificate("TX test non CA", root_cert.subject,
        non_ca_key.public_key(), root_key, 105, now,
        -timedelta(days=1), timedelta(days=30), False)
    non_ca_leaf = certificate("service.example", non_ca_cert.subject,
        leaf_key.public_key(), non_ca_key, 106, now,
        -timedelta(days=1), timedelta(days=30), False, True)
    signing_cert = certificate("TX code signer", middle_cert.subject,
        leaf_key.public_key(), middle_key, 107, now,
        -timedelta(days=1), timedelta(days=30), False, signing=True)
    for name, cert in (("root", root_cert), ("intermediate", middle_cert),
                       ("leaf", leaf_cert), ("expired", expired_cert),
                       ("future", future_cert), ("non_ca", non_ca_cert),
                       ("non_ca_leaf", non_ca_leaf), ("signing", signing_cert)):
        (directory / f"{name}.der").write_bytes(cert.public_bytes(
            serialization.Encoding.DER))
        (directory / f"{name}.pem").write_bytes(cert.public_bytes(
            serialization.Encoding.PEM))
    (directory / "bundle.pem").write_bytes(b"".join(cert.public_bytes(
        serialization.Encoding.PEM) for cert in
        (leaf_cert, middle_cert, root_cert)))
    (directory / "many.pem").write_bytes(root_cert.public_bytes(
        serialization.Encoding.PEM) * 65)
    package = pkcs12.serialize_key_and_certificates(
        b"tx-test-identity", leaf_key, leaf_cert, [middle_cert, root_cert],
        serialization.BestAvailableEncryption(b"test-only-password"))
    (directory / "identity.p12").write_bytes(package)
    return leaf_key.public_key().public_numbers()


def run(*command):
    values = environment()
    result = subprocess.run([str(item) for item in command], cwd=root,
                            env=values, capture_output=True, timeout=90)
    output = (result.stdout + result.stderr).decode("utf-8", errors="replace")
    if result.returncode:
        raise AssertionError(f"exit={result.returncode}\n{output}")
    return output.strip()


def main():
    with tempfile.TemporaryDirectory(prefix="tx-x509-") as location:
        directory = Path(location)
        expected_public = fixtures(directory)
        program = directory / "x509_check.exe"
        run(TXC, root / "tests/crypto/x509.tx", "-o", program)
        exported = directory / "private-key.pk8"
        print(run(program, directory, exported))
        actual = serialization.load_der_private_key(exported.read_bytes(),
                                                     password=None)
        assert actual.public_key().public_numbers() == expected_public
        exported.unlink()
        print("X509_PKCS8_INTEROP_OK")
        example = directory / "x509_example.exe"
        run(TXC, root / "examples/x509.tx", "-o", example)
        print(run(example, directory / "leaf.pem", directory / "intermediate.pem",
                  directory / "root.pem"))
        native = directory / "x509_native.exe"
        native_libraries = ([f"-L{TOOL_DIR / 'link'}", "-lcrypt32", "-lncrypt",
            "-ladvapi32", "-lbcrypt", "-lwinhttp", "-lws2_32", "-lshell32",
            "-luser32", "-liconv"] if os.name == "nt" else
            [f"-L{TOOL_DIR / 'lib'}", f"-L{TOOL_DIR / 'link'}",
             "-licui18n", "-licuuc", "-licudata", "-lpcre2-8", "-lsodium",
             "-largon2", "-lxml2", "-lpq", "-lnghttp2", "-lcurl", "-lcares",
             "-lssl", "-lcrypto", "-lz", "-ldl", "-pthread"])
        run("g++", "-std=c++23", "-O2", "-finput-charset=UTF-8",
            "-fexec-charset=UTF-8", "-Isrc", root / "tests/crypto/x509_native.cpp",
            TOOL_DIR / "libtxstdlib.a", *native_libraries, "-o", native)
        print(run(native, directory))


if __name__ == "__main__":
    main()
