"""隔离的 Linux 本机服务和秘密夹具；不打印秘密值。"""
from contextlib import contextmanager
from pathlib import Path
import importlib.util
import os
import secrets
import socket
import ssl
import subprocess
import tempfile
import threading
import uuid


def load(path):
    spec = importlib.util.spec_from_file_location(path.stem, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


@contextmanager
def network(folder):
    stop = threading.Event()
    udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp.bind(("127.0.0.1", 0))
    tcp = socket.socket()
    tcp.bind(("127.0.0.1", 0))
    tcp.listen()
    ipc = socket.socket(socket.AF_UNIX)
    name = "bench_" + uuid.uuid4().hex
    ipc.bind("\0tx-ipc-" + str(os.geteuid()) + "-" + name)
    ipc.listen()
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.minimum_version = context.maximum_version = ssl.TLSVersion.TLSv1_2
    context.options |= ssl.OP_NO_TICKET
    context.load_cert_chain(folder / "server.crt", folder / "server.key")
    context.set_alpn_protocols(["bench"])
    failures = []

    def echo(server, kind):
        server.settimeout(0.2)
        while not stop.is_set():
            try:
                if kind == "udp":
                    data, address = server.recvfrom(8192)
                    server.sendto(data, address)
                    continue
                peer, _ = server.accept()
                with peer:
                    peer.settimeout(5)
                    if kind == "tls":
                        peer.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                        with context.wrap_socket(peer, server_side=True) as secure:
                            secure.sendall(secure.recv(1))
                    else:
                        while data := peer.recv(65536):
                            peer.sendall(data)
            except socket.timeout:
                continue
            except OSError as error:
                if not stop.is_set():
                    failures.append(type(error).__name__)
    threads = [threading.Thread(target=echo, args=(server, kind), daemon=True)
               for server, kind in ((udp, "udp"), (tcp, "tls"), (ipc, "ipc"))]
    for thread in threads:
        thread.start()
    try:
        yield {"BENCH_UDP_PORT": str(udp.getsockname()[1]), "BENCH_TLS_PORT": str(tcp.getsockname()[1]), "BENCH_PIPE": name}
        if failures:
            raise RuntimeError("回声服务异常: " + str(failures))
    finally:
        stop.set()
        for server in (udp, tcp, ipc):
            server.close()
        for thread in threads:
            thread.join(6)


@contextmanager
def postgres(database, command):
    with tempfile.TemporaryDirectory(prefix="tx_linux_perf_pg_") as directory:
        folder = Path(directory)
        data = folder / "data"
        binaries = Path("/usr/lib/postgresql/16/bin")
        password = secrets.token_hex(24)
        password_file = folder / "password"
        password_file.write_text(password, encoding="utf-8")
        password_file.chmod(0o600)
        command([binaries / "initdb", "-D", data, "-U", "tx_test", "--encoding=UTF8", "--no-locale", "--auth=scram-sha-256", "--pwfile", password_file])
        password_file.unlink()
        database.certificates(data)
        (data / "server.key").chmod(0o600)
        with socket.socket() as listener:
            listener.bind(("127.0.0.1", 0))
            port = listener.getsockname()[1]
        with (data / "postgresql.conf").open("a", encoding="utf-8") as config:
            config.write(f"\nlisten_addresses='127.0.0.1'\nport={port}\nssl=on\nssl_cert_file='server.crt'\nssl_key_file='server.key'\nunix_socket_directories='{folder}'\n")
        try:
            command([binaries / "pg_ctl", "-D", data, "-l", folder / "server.log", "-w", "start"])
            yield {"BENCH_GROUP": "postgres", "TX_DB_PORT": str(port), "TX_DB_CA": str(data / "ca.pem"), "TX_DB_PASSWORD_HEX": password.encode().hex()}
        finally:
            if (data / "postmaster.pid").exists():
                command([binaries / "pg_ctl", "-D", data, "-m", "immediate", "-w", "stop"])


def extended(suite, root, work, command):
    source = root / "benchmarks/stdlib_retest_2026-09-30_static_execution"
    database = load(root / "scripts/check_db_postgres.py")
    with tempfile.TemporaryDirectory(prefix="fixtures_", dir=work) as directory:
        folder = Path(directory)
        database.certificates(folder)
        from cryptography import x509
        from cryptography.hazmat.primitives import serialization
        from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
        for name, original in (("ca.der", "ca.pem"), ("server.der", "server.crt")):
            certificate = x509.load_pem_x509_certificate((folder / original).read_bytes())
            (folder / name).write_bytes(certificate.public_bytes(serialization.Encoding.DER))
        (folder / "ed_seed.bin").write_bytes(Ed25519PrivateKey.generate().private_bytes(serialization.Encoding.Raw, serialization.PrivateFormat.Raw, serialization.NoEncryption()))
        (folder / "message.bin").write_bytes(bytes(range(256)) * 4)
        (folder / "block.bin").write_bytes(bytes(range(256)) * 256)
        (folder / "password.bin").write_bytes(os.urandom(24))
        for group, expected in {
            "concurrency": {"thread_spawn_join": 100, "mutex_uncontended": 100000, "atomic_add": 100000, "channel_send_recv": 200010000, "task_spawn_wait": 500},
            "sqlite": {"sqlite_insert": 2000, "sqlite_read": 20150000, "sqlite_savepoint": 100, "sqlite_pool": 4200, "sqlite_async": 4200, "migration_recheck": 100},
            "security": {"secret_equal": 20000, "argon2_hash_verify": 3, "ed25519_sign": 32000, "ed25519_verify": 500, "x509_parse_der": 500},
            "async_file": {"async_file_rw": 4194304},
            "diagnostics": {"test_parameterized": 199990000, "test_property": 20000, "log_filtered": 100000, "log_file": 2000},
            "profile": {"profile_spans": 1000},
        }.items():
            suite(group, source / (("database" if group == "sqlite" else group) + ".tx"), cwd=folder,
                  env={**os.environ, "BENCH_GROUP": group}, expected={k: str(v) for k, v in expected.items()})
        with network(folder) as environment:
            suite("network", source / "network.tx", cwd=folder, env={**os.environ, **environment},
                  expected={"dns_localhost": "100", "udp_echo": "512000", "ipc_echo": "21000", "tls_handshake": "10"})
        with postgres(database, command) as environment:
            suite("postgres", source / "database.tx", cwd=folder, env={**os.environ, **environment},
                  expected={"postgres_insert": "2000", "postgres_read": "20150000", "postgres_savepoint": "100"})
