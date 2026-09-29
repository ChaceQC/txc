"""定向验证跨线程 TX 错误快照与无错误空结果的 ABI 兜底。"""

from pathlib import Path
import os
import shutil
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "tx_build"


def run(command: list[str], *, environment: dict[str, str], timeout: int) -> str:
    result = subprocess.run(command, cwd=ROOT, env=environment,
                            capture_output=True, timeout=timeout)
    output = (result.stdout + result.stderr).decode("utf-8", errors="replace")
    if result.returncode:
        raise AssertionError(f"exit={result.returncode}\n{output}")
    return output.strip()


def main() -> None:
    sys.stdout.reconfigure(encoding="utf-8")
    compiler = shutil.which("g++")
    if compiler is None:
        raise RuntimeError("未找到 g++")
    environment = os.environ.copy()
    environment["PATH"] = str(ROOT / "tx") + os.pathsep + environment["PATH"]
    OUTPUT.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="concurrency_errors_", dir=OUTPUT) as folder:
        work = Path(folder)
        tx_program = work / "concurrency_error_propagation.exe"
        run([str(ROOT / "tx/txc.exe"),
             str(ROOT / "tests/stdlib/concurrency_error_propagation.tx"),
             "-o", str(tx_program)], environment=environment, timeout=90)
        run([str(tx_program)], environment=environment, timeout=30)
        print("TX thread.join/task.wait 原始错误与后续正常任务: PASS")

        abi_program = work / "concurrency_error_abi.exe"
        run([compiler, "-std=c++23", "-O0", "-pthread", "-Isrc",
             str(ROOT / "tests/stdlib/concurrency_error_abi.cpp"),
             "tx/libtxstdlib.a", "-Ltx/link", "-lwinhttp", "-lws2_32",
             "-ldnsapi", "-ladvapi32", "-lbcrypt", "-lcrypt32",
             "-lncrypt", "-lshell32", "-luser32", "-liconv",
             "-o", str(abi_program)], environment=environment, timeout=90)
        print(run([str(abi_program)], environment=environment, timeout=30))


if __name__ == "__main__":
    main()
