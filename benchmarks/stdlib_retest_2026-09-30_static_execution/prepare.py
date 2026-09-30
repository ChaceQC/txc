"""准备隔离夹具与固定版本参考依赖，不显示秘密材料。"""
from pathlib import Path
import importlib.util
import json
import os
import subprocess
import sys
import urllib.request

root = Path(__file__).resolve().parents[2]
source = Path(__file__).resolve().parent
work = root / "tx_build/stdlib_retest_2026_09_30_static_execution"
jars = {
    "sqlite-jdbc-3.46.1.0.jar": "org/xerial/sqlite-jdbc/3.46.1.0/sqlite-jdbc-3.46.1.0.jar",
    "postgresql-42.7.4.jar": "org/postgresql/postgresql/42.7.4/postgresql-42.7.4.jar",
    "bcprov-jdk18on-1.78.1.jar": "org/bouncycastle/bcprov-jdk18on/1.78.1/bcprov-jdk18on-1.78.1.jar",
    "slf4j-api-2.0.16.jar": "org/slf4j/slf4j-api/2.0.16/slf4j-api-2.0.16.jar",
}


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def command(args, env=None, timeout=180):
    result = subprocess.run(list(map(str, args)), cwd=root, env=env, capture_output=True,
                            encoding="utf-8", errors="replace", timeout=timeout)
    if result.returncode:
        message = result.stdout[-3000:] + result.stderr[-3000:]
        if env:
            for key in ("TX_DB_PASSWORD", "TX_DB_PASSWORD_HEX"):
                if env.get(key):
                    message = message.replace(env[key], "[REDACTED]")
        raise RuntimeError(message)
    return result.stdout


def main():
    work.mkdir(parents=True, exist_ok=True)
    dependencies = work / "dependencies"
    dependencies.mkdir(exist_ok=True)
    for name, path in jars.items():
        target = dependencies / name
        if not target.exists():
            with urllib.request.urlopen("https://repo.maven.apache.org/maven2/" + path, timeout=60) as response:
                target.write_bytes(response.read())
        print("READY dependency", name, flush=True)
    python_dependencies = dependencies / "python"
    if not (python_dependencies / "psycopg").exists():
        command([sys.executable, "-m", "pip", "install", "--disable-pip-version-check", "--target",
                 python_dependencies, "psycopg[binary]==3.2.3"], timeout=240)
    classpath = os.pathsep.join([str(work), *map(str, dependencies.glob("*.jar"))])
    command(["javac", "-encoding", "UTF-8", "-cp", classpath, "-d", work, *source.glob("*.java")])
    print("READY Java", flush=True)
    command(["g++", "-std=c++23", "-O3", "-pthread", "-finput-charset=UTF-8", "-fexec-charset=UTF-8",
             "-Isrc", *source.glob("*.cpp"), root / "tx/libtxstdlib.a", "-Ltx/link", "-lwinhttp", "-lws2_32",
             "-ldnsapi", "-ladvapi32", "-lbcrypt", "-lcrypt32", "-lncrypt", "-lshell32", "-luser32",
             "-liconv", "-lpsapi", "-o", work / "reference_cpp.exe"])
    print("READY C++", flush=True)
    for path in source.glob("*.tx"):
        command([root / "tx/txc.exe", path, "-o", work / (path.stem + ".exe")])
        print("READY TX", path.name, flush=True)
    fixtures = work / "fixtures"
    fixtures.mkdir(exist_ok=True)
    database = load("database_fixture", root / "scripts/check_db_postgres.py")
    database.certificates(fixtures)
    from cryptography import x509
    from cryptography.hazmat.primitives import serialization
    from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
    for name, original in (("ca.der", "ca.pem"), ("server.der", "server.crt")):
        certificate = x509.load_pem_x509_certificate((fixtures / original).read_bytes())
        (fixtures / name).write_bytes(certificate.public_bytes(serialization.Encoding.DER))
    private = Ed25519PrivateKey.generate()
    (fixtures / "ed_seed.bin").write_bytes(private.private_bytes_raw())
    (fixtures / "ed_private.der").write_bytes(private.private_bytes(serialization.Encoding.DER,
        serialization.PrivateFormat.PKCS8, serialization.NoEncryption()))
    (fixtures / "ed_public.der").write_bytes(private.public_key().public_bytes(serialization.Encoding.DER,
        serialization.PublicFormat.SubjectPublicKeyInfo))
    (fixtures / "message.bin").write_bytes(bytes(range(256)) * 4)
    (fixtures / "block.bin").write_bytes(bytes(range(256)) * 256)
    (fixtures / "password.bin").write_bytes(os.urandom(24))
    print("READY isolated fixtures; secret values omitted", flush=True)


if __name__ == "__main__":
    main()
