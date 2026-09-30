"""Python 独立参考：标准库、cryptography、argon2-cffi、psycopg。"""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import ctypes
import hashlib
import hmac
import json
import os
import queue
import socket
import sqlite3
import ssl
import struct
import sys
import threading
import time


def report(name, start, total):
    print(name, (time.perf_counter_ns() - start) / 1000, total, sep="\n")


def concurrency():
    total = [0]
    def one():
        total[0] += 1
    start = time.perf_counter_ns()
    for _ in range(100):
        worker = threading.Thread(target=one)
        worker.start()
        worker.join()
    report("thread_spawn_join", start, total[0])
    lock = threading.Lock()
    total = 0
    start = time.perf_counter_ns()
    for _ in range(100000):
        with lock:
            total += 1
    report("mutex_uncontended", start, total)
    # CPython 没有公开的 int64 原子 RMW，使用锁实现相同的线程安全加法并在报告注明。
    value = 0
    start = time.perf_counter_ns()
    for _ in range(100000):
        with lock:
            value += 1
    report("atomic_add", start, value)
    channel = queue.Queue(1)
    total = 0
    start = time.perf_counter_ns()
    for i in range(1, 20001):
        channel.put(i, timeout=1)
        total += channel.get(timeout=1)
    report("channel_send_recv", start, total)
    start = time.perf_counter_ns()
    with ThreadPoolExecutor(1) as executor:
        total = sum(executor.submit(lambda: 1).result() for _ in range(500))
    report("task_spawn_wait", start, total)


def database(postgres=False):
    if postgres:
        import psycopg
        connection = psycopg.connect(host="localhost", port=os.environ["TX_DB_PORT"],
            dbname="postgres", user="tx_test", password=os.environ["TX_DB_PASSWORD"],
            sslmode="verify-full", sslrootcert=os.environ["TX_DB_CA"], autocommit=True)
    else:
        connection = sqlite3.connect(":memory:", isolation_level=None)
    prefix = "postgres" if postgres else "sqlite"
    marker = "%s" if postgres else "?"
    connection.execute("CREATE TEMP TABLE bench_items(id BIGINT, label TEXT)")
    connection.execute("BEGIN")
    cursor = connection.cursor()
    start = time.perf_counter_ns()
    total = 0
    for i in range(1, 2001):
        cursor.execute(f"INSERT INTO bench_items VALUES({marker}, 'payload')", (i,))
        total += cursor.rowcount
    connection.execute("COMMIT")
    report(prefix + "_insert", start, total)
    start = time.perf_counter_ns()
    total = 0
    for _ in range(10):
        cursor.execute("SELECT id, label FROM bench_items ORDER BY id")
        for row in cursor:
            total += row[0] + len(row[1])
    report(prefix + "_read", start, total)
    start = time.perf_counter_ns()
    for _ in range(100):
        connection.execute("BEGIN")
        connection.execute("SAVEPOINT point")
        connection.execute("INSERT INTO bench_items VALUES(9999, 'rollback')")
        connection.execute("ROLLBACK TO point")
        connection.execute("RELEASE point")
        connection.execute("COMMIT")
    report(prefix + "_savepoint", start, 100)
    assert connection.execute("SELECT count(*) FROM bench_items").fetchone()[0] == 2000
    connection.close()
    if postgres:
        return
    def query(transaction=False):
        # TX SQLite 池在归还时关闭并重开；参考同样每次连接，避免把复用优势混入倍率。
        conn = sqlite3.connect("bench.sqlite", isolation_level=None)
        if transaction:
            conn.execute("BEGIN")
        value = conn.execute("SELECT 42").fetchone()[0]
        if transaction:
            conn.execute("COMMIT")
        conn.close()
        return value
    start = time.perf_counter_ns()
    total = sum(query() for _ in range(100))
    report("sqlite_pool", start, total)
    with ThreadPoolExecutor(1) as executor:
        start = time.perf_counter_ns()
        total = sum(executor.submit(query, True).result() for _ in range(100))
        report("sqlite_async", start, total)
    connection = sqlite3.connect(":memory:", isolation_level=None)
    sql = "CREATE TABLE migration_items(id BIGINT)"
    expected = hashlib.sha256(f"tx-migration-v1\n{len(sql)}:{sql}".encode()).hexdigest()
    connection.execute("CREATE TABLE tx_schema_migrations(version BIGINT PRIMARY KEY, checksum TEXT NOT NULL)")
    connection.execute("INSERT INTO tx_schema_migrations VALUES(1, ?)", (expected,))
    connection.execute(sql)
    def history():
        connection.execute("BEGIN IMMEDIATE")
        connection.execute("CREATE TABLE IF NOT EXISTS tx_schema_migrations(version BIGINT PRIMARY KEY, checksum TEXT NOT NULL)")
        rows = connection.execute("SELECT version, checksum FROM tx_schema_migrations ORDER BY version").fetchall()
        assert rows == [(1, expected)]
        connection.execute("COMMIT")
        return len(rows)
    start = time.perf_counter_ns()
    total = 0
    for _ in range(100):
        assert hashlib.sha256(f"tx-migration-v1\n{len(sql)}:{sql}".encode()).hexdigest() == expected
        history()
        total += history()
    report("migration_recheck", start, total)
    connection.close()


def security():
    from argon2.low_level import hash_secret, verify_secret, Type
    from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
    from cryptography import x509
    from cryptography.hazmat.primitives.serialization import Encoding
    data = Path("message.bin").read_bytes()
    other = bytes(bytearray(data))
    start = time.perf_counter_ns()
    total = sum(hmac.compare_digest(data, other) for _ in range(20000))
    report("secret_equal", start, total)
    credential = Path("password.bin").read_bytes()
    start = time.perf_counter_ns()
    total = 0
    for _ in range(3):
        encoded = hash_secret(credential, os.urandom(16), 2, 19456, 1, 32, Type.ID)
        total += verify_secret(encoded, credential, Type.ID)
    report("argon2_hash_verify", start, total)
    key = Ed25519PrivateKey.from_private_bytes(Path("ed_seed.bin").read_bytes())
    pub = key.public_key()
    start = time.perf_counter_ns()
    total = 0
    for _ in range(500):
        signature = key.sign(data)
        total += len(signature)
    report("ed25519_sign", start, total)
    start = time.perf_counter_ns()
    for _ in range(500):
        pub.verify(signature, data)
    report("ed25519_verify", start, 500)
    cert = Path("server.der").read_bytes()
    start = time.perf_counter_ns()
    total = sum(len(x509.load_der_x509_certificate(cert).public_bytes(Encoding.DER)) for _ in range(500))
    report("x509_parse_der", start, total)


def network():
    start = time.perf_counter_ns()
    total = sum(bool(socket.getaddrinfo("localhost", None, type=socket.SOCK_STREAM)) for _ in range(100))
    report("dns_localhost", start, total)
    data = Path("message.bin").read_bytes()
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as udp:
        udp.settimeout(5)
        total = 0
        start = time.perf_counter_ns()
        for _ in range(500):
            udp.sendto(data, ("127.0.0.1", int(os.environ["BENCH_UDP_PORT"])))
            reply, _ = udp.recvfrom(2048)
            assert reply == data
            total += len(reply)
        report("udp_echo", start, total)
    with open("\\\\.\\pipe\\tx-ipc-" + os.environ["BENCH_PIPE"], "r+b", buffering=0) as pipe:
        start = time.perf_counter_ns()
        total = 0
        for _ in range(500):
            frame = b"TXIP" + struct.pack("<IQI", 1, 1, 2) + bytes([0x18, 42])
            pipe.write(frame)
            reply = b""
            while len(reply) < 22:
                reply += pipe.read(22 - len(reply))
            assert reply == frame
            total += reply[-1]
        report("ipc_echo", start, total)
    context = ssl.create_default_context(cafile="ca.pem")
    context.set_alpn_protocols(["bench"])
    total = 0
    start = time.perf_counter_ns()
    for _ in range(10):
        with socket.create_connection(("127.0.0.1", int(os.environ["BENCH_TLS_PORT"])), 5) as raw:
            with context.wrap_socket(raw, server_hostname="localhost") as secure:
                secure.sendall(b"*")
                assert secure.recv(1) == b"*"
                total += 1
    report("tls_handshake", start, total)


def async_file():
    payload = Path("block.bin").read_bytes()
    def write(index):
        with open("async.bin", "r+b" if Path("async.bin").exists() else "w+b", buffering=0) as file:
            file.seek(index * 65536)
            return file.write(payload)
    def read(index):
        with open("async.bin", "rb", buffering=0) as file:
            file.seek(index * 65536)
            data = file.read(65536)
            assert data == payload
            return len(data)
    with ThreadPoolExecutor(1) as executor:
        start = time.perf_counter_ns()
        total = 0
        for index in range(32):
            total += executor.submit(write, index).result()
            total += executor.submit(read, index).result()
        report("async_file_rw", start, total)


def diagnostics():
    total = [0]
    def parameter(index):
        total[0] += index
    start = time.perf_counter_ns()
    for index in range(20000):
        parameter(index)
    report("test_parameterized", start, total[0])
    total[0] = 0
    def generate(seed, index):
        return seed + index
    def predicate(value):
        total[0] += 1
        return value >= 1000
    start = time.perf_counter_ns()
    for index in range(20000):
        assert predicate(generate(1000, index))
    report("test_property", start, total[0])
    import logging
    logger = logging.getLogger("benchmark")
    logger.setLevel(logging.INFO)
    start = time.perf_counter_ns()
    for _ in range(100000):
        if logger.isEnabledFor(logging.DEBUG):
            logger.debug("filtered %s", {"index": 42})
    report("log_filtered", start, 100000)
    lock = threading.Lock()
    with open("events.jsonl", "w", encoding="utf-8") as file:
        start = time.perf_counter_ns()
        for index in range(1, 2001):
            fields = {"index": index, "password": "synthetic"}
            fields["password"] = "[REDACTED]"
            line = json.dumps({"timestamp_ms": time.time_ns() // 1000000,
                "level": "info", "message": "entry", "context": {"request_id": "bench",
                "task_id": "", "thread_id": str(threading.get_ident())}, "fields": fields})
            with lock:
                file.write(line + "\n")
                file.flush()
        file.flush()
        report("log_file", start, 2000)


def profile():
    records = []
    start = time.perf_counter_ns()
    for index in range(1000):
        begin = time.perf_counter_ns()
        records.append({"id": index, "name": "bench", "duration": time.perf_counter_ns() - begin})
    report("profile_spans", start, len(records))


if __name__ == "__main__":
    group = sys.argv[1]
    if group in ("sqlite", "postgres"):
        database(group == "postgres")
    else:
        globals()[group]()
