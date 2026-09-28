"""丢弃本机 UDP 握手包，验证活动 HTTP/3 握手被取消令牌及时中断。"""

import pathlib
import socket
import subprocess
import sys
import threading
import time


ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.stdout.reconfigure(encoding="utf-8")


def main():
    source = ROOT / "tests/network/http3_cancel.tx"
    subprocess.run([ROOT / "tx/txc.exe", source], cwd=ROOT, check=True)
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sink:
        sink.bind(("127.0.0.1", 19752))
        sink.settimeout(0.2)
        stopped = threading.Event()

        def discard():
            while not stopped.is_set():
                try:
                    sink.recv(65536)
                except socket.timeout:
                    pass

        worker = threading.Thread(target=discard, daemon=True)
        worker.start()
        try:
            started = time.perf_counter()
            result = subprocess.run([ROOT / "tx_build/http3_cancel.exe"],
                cwd=ROOT, capture_output=True, timeout=4)
            elapsed = time.perf_counter() - started
            if result.returncode != 0 or elapsed >= 1.5:
                raise AssertionError((result.stdout + result.stderr)
                    .decode("utf-8", "replace") + f" 耗时 {elapsed:.3f}s")
            print(f"HTTP/3 握手取消通过：{elapsed:.3f}s")
        finally:
            stopped.set()
            worker.join()


if __name__ == "__main__":
    main()
