import os
import subprocess
import time


def measure(name, operation):
    start = time.perf_counter_ns()
    checksum = operation()
    elapsed = (time.perf_counter_ns() - start) // 1000
    print(name)
    print(elapsed)
    print(checksum)


def file_stream_rw():
    path = "tx_build/perf_audit_20260927/stream_python.bin"
    data = b"0123456789abcdef" * 4
    checksum = 0
    for _ in range(200):
        with open(path, "wb") as output:
            output.write(data)
        with open(path, "rb") as source:
            checksum += len(source.read())
    return checksum


def process_spawn():
    path = os.path.abspath("tx_build/perf_audit_20260927/child.exe")
    checksum = 0
    for _ in range(50):
        result = subprocess.run(
            [path], stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL, check=False,
        )
        checksum += int(result.returncode == 0)
    return checksum


if __name__ == "__main__":
    measure("file_stream_rw", file_stream_rw)
    measure("process_spawn", process_spawn)
