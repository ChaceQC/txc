import os
from pathlib import Path
import sys
import time


def measure(name, operation):
    start = time.perf_counter_ns()
    checksum = operation()
    elapsed = (time.perf_counter_ns() - start) / 1_000_000
    print(name)
    print(elapsed)
    print(checksum)


def string_case():
    checksum = 0
    for _ in range(10000):
        checksum += len("alpha,beta,alpha".replace("alpha", "x").split(","))
    return checksum


def array_case():
    values = list(range(64))
    checksum = 0
    for _ in range(3000):
        combined = values + values
        reversed_values = list(reversed(combined))
        middle = reversed_values[16:112]
        checksum += middle[0] + len(middle)
    return checksum


def dict_case():
    values = {}
    checksum = 0
    for repetition in range(1, 101):
        for i in range(128):
            values[i] = i + repetition
            checksum += values[i]
    return checksum


def path_case():
    checksum = 0
    for _ in range(10000):
        joined = Path("tx_build") / "sub/data.txt"
        checksum += len(joined.suffix)
    return checksum


def fs_case():
    checksum = 0
    for _ in range(200):
        entries = sorted(os.listdir("tx/stdlib"))
        checksum += len(entries)
        if os.path.isfile("tx/stdlib/math.txh"):
            checksum += 1
    return checksum


def file_case():
    content = "0123456789abcdef" * 4
    path = "tx_build/perf_audit_20260927/legacy_python.txt"
    checksum = 0
    for _ in range(300):
        with open(path, "w", encoding="utf-8") as output:
            output.write(content)
        with open(path, "r", encoding="utf-8") as source:
            checksum += len(source.read())
    return checksum


def io_case():
    for _ in range(5000):
        sys.stderr.write("x")
    return 5000


def time_case():
    checksum = 0
    for _ in range(100000):
        checksum += int(time.time_ns() // 1_000_000 > 0)
    return checksum


if __name__ == "__main__":
    for name, operation in (
        ("string", string_case), ("array", array_case),
        ("dict_hash", dict_case), ("path", path_case),
        ("fs", fs_case), ("file", file_case),
        ("io", io_case), ("time", time_case),
    ):
        measure(name, operation)
