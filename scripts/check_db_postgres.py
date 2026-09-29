"""12.3/12.4 定向验证：仅启动工作区临时 PostgreSQL，不连接已有数据库。"""

from contextlib import contextmanager
from datetime import datetime, timedelta, timezone
from pathlib import Path
import hashlib
import os
import secrets
import shutil
import socket
import subprocess
import tempfile
import sys
import zipfile

from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.x509.oid import NameOID, ExtendedKeyUsageOID

ROOT = Path(__file__).resolve().parents[1]
DIGEST = "7effe34c0bf89027b3f171447d351cbc460f4566c8d0f643daec67f140787858"


def run(command, folder, environment, timeout=90):
    result = subprocess.run([str(part) for part in command], cwd=folder,
                            env=environment, capture_output=True, text=True,
                            encoding="utf-8", errors="replace", timeout=timeout,
                            creationflags=subprocess.CREATE_NO_WINDOW)
    if result.returncode:
        # 不输出配置、环境或完整调用参数；运行时错误本身已经过库内遮蔽。
        raise RuntimeError(f"{Path(command[0]).name}: exit={result.returncode}\n"
                           + result.stdout + result.stderr)
    return result.stdout


def server_command(command, folder, environment):
    # Windows 后台 postgres 会继承 pg_ctl 的管道句柄，不能使用 communicate 捕获。
    result = subprocess.run([str(part) for part in command], cwd=folder, env=environment,
                            stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL, timeout=30,
                            creationflags=subprocess.CREATE_NO_WINDOW)
    if result.returncode:
        raise RuntimeError(f"临时数据库控制失败：exit={result.returncode}")


def server_root():
    for folder in (ROOT / "build/_deps/postgres_binary-src",
                   ROOT / "build/_deps/postgres_binary-src/pgsql"):
        if (folder / "bin/initdb.exe").exists():
            return folder
    archive = ROOT / "tx_build/postgresql-18.4-1.zip"
    if not archive.exists() or hashlib.file_digest(archive.open("rb"), "sha256").hexdigest() != DIGEST:
        raise RuntimeError("需要固定 PostgreSQL 18.4-1 构建依赖；先运行增量构建")
    target = ROOT / "tx_build/postgres_test_server"
    with zipfile.ZipFile(archive) as source:
        for entry in source.infolist():
            if entry.filename.startswith(("pgsql/bin/", "pgsql/share/", "pgsql/lib/")):
                source.extract(entry, target)
    return target / "pgsql"


def certificates(folder):
    now = datetime.now(timezone.utc)

    def authority(common_name):
        key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
        name = x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, common_name)])
        certificate = (x509.CertificateBuilder().subject_name(name).issuer_name(name)
                       .public_key(key.public_key()).serial_number(x509.random_serial_number())
                       .not_valid_before(now - timedelta(days=1)).not_valid_after(now + timedelta(days=2))
                       .add_extension(x509.BasicConstraints(ca=True, path_length=None), critical=True)
                       .sign(key, hashes.SHA256()))
        return key, certificate

    ca_key, ca = authority("TX temporary database CA")
    _, wrong = authority("TX unrelated CA")
    server_key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
    certificate = (x509.CertificateBuilder()
                   .subject_name(x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, "localhost")]))
                   .issuer_name(ca.subject).public_key(server_key.public_key())
                   .serial_number(x509.random_serial_number())
                   .not_valid_before(now - timedelta(days=1)).not_valid_after(now + timedelta(days=2))
                   .add_extension(x509.SubjectAlternativeName([x509.DNSName("localhost")]), critical=False)
                   .add_extension(x509.ExtendedKeyUsage([ExtendedKeyUsageOID.SERVER_AUTH]), critical=False)
                   .sign(ca_key, hashes.SHA256()))
    for name, cert in (("ca.pem", ca), ("wrong-ca.pem", wrong), ("server.crt", certificate)):
        (folder / name).write_bytes(cert.public_bytes(serialization.Encoding.PEM))
    (folder / "server.key").write_bytes(server_key.private_bytes(
        serialization.Encoding.PEM, serialization.PrivateFormat.TraditionalOpenSSL,
        serialization.NoEncryption()))


@contextmanager
def temporary_server(folder, server):
    environment = {key: value for key, value in os.environ.items() if not key.startswith("PG")}
    environment["PATH"] = str(server / "bin") + os.pathsep + environment["PATH"]
    password = secrets.token_hex(24)
    password_file = folder / "password"
    password_file.write_text(password, encoding="utf-8")
    data = folder / "data"
    run([server / "bin/initdb.exe", "-D", data, "-U", "tx_test", "--encoding=UTF8",
         "--no-locale", "--auth=scram-sha-256", "--pwfile", password_file], folder, environment)
    password_file.unlink()
    certificates(data)
    with socket.socket() as listener:
        listener.bind(("127.0.0.1", 0))
        port = listener.getsockname()[1]
    with (data / "postgresql.conf").open("a", encoding="utf-8") as config:
        config.write(f"\nlisten_addresses='127.0.0.1'\nport={port}\nssl=on\n"
                     "ssl_cert_file='server.crt'\nssl_key_file='server.key'\n"
                     "password_encryption='scram-sha-256'\n")
    environment.update(TX_DB_PORT=str(port), TX_DB_CA=str(data / "ca.pem"),
                       TX_DB_WRONG_CA=str(data / "wrong-ca.pem"), TX_DB_PASSWORD=password,
                       TX_DB_PASSWORD_HEX=password.encode().hex())
    try:
        server_command([server / "bin/pg_ctl.exe", "-D", data, "-l", folder / "server.log", "-w", "start"],
                       folder, environment)
        yield environment
    finally:
        if (data / "postmaster.pid").exists():
            server_command([server / "bin/pg_ctl.exe", "-D", data, "-m", "immediate", "-w", "stop"],
                           folder, environment)


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    groups = set(sys.argv[1:] or ["postgres", "pool", "native", "example"])
    if groups - {"postgres", "pool", "native", "example"}:
        raise SystemExit("用法：python scripts/check_db_postgres.py [postgres] [pool] [native] [example]")
    server = server_root()
    with tempfile.TemporaryDirectory(prefix="db_postgres_", dir=ROOT / "tx_build") as temporary:
        folder = Path(temporary)
        with temporary_server(folder, server) as environment:
            environment["PATH"] = str(ROOT / "tx") + os.pathsep + environment["PATH"]
            for source in ("postgres.tx", "pool.tx"):
                if Path(source).stem not in groups:
                    continue
                executable = folder / (source + ".exe")
                run([ROOT / "tx/txc.exe", ROOT / "tests/db" / source, "-o", executable],
                    ROOT, environment, timeout=180)
                print(run([executable], folder, environment, timeout=30).strip())
            if "example" in groups:
                run([ROOT / "tx/txc.exe", "check", ROOT / "examples/postgres.tx"], ROOT, environment)
            if "native" not in groups:
                return
            executable = folder / "postgres_native.exe"
            run([shutil.which("g++"), "-std=c++23", "-O0", "-pthread", "-Isrc",
                 ROOT / "tests/db/postgres_native.cpp", "tx/libtxstdlib.a", "-Ltx/link",
                 "-lwinhttp", "-lws2_32", "-ldnsapi", "-ladvapi32", "-lbcrypt", "-lcrypt32",
                 "-lncrypt", "-lshell32", "-luser32", "-liconv", "-o", executable],
                ROOT, environment, timeout=90)
            print(run([executable], folder, environment, timeout=35).strip())
    print("12.3/12.4 PostgreSQL 和连接池定向验证全部通过")


if __name__ == "__main__":
    main()
