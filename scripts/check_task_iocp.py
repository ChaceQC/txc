"""验证 IOCP I/O 的提交、取消和恰好一次完成路径。"""

from pathlib import Path
import os
import shutil
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "tx_build"
SOURCE = ROOT / "tests/stdlib/task_iocp_state_machine.cpp"
sys.stdout.reconfigure(encoding="utf-8")


def run(command: list[str], timeout: int, *, cwd: Path = ROOT,
        env: dict[str, str] | None = None) -> str:
    result = subprocess.run(command, cwd=cwd, env=env, capture_output=True,
                            timeout=timeout)
    output = (result.stdout + result.stderr).decode("utf-8", errors="replace")
    if result.returncode:
        raise AssertionError(f"exit={result.returncode}\n{output}")
    return output.strip()


def main() -> None:
    if os.name != "nt":
        raise RuntimeError("IOCP 状态机验收需要 Windows")
    compiler = shutil.which("g++")
    if compiler is None:
        raise RuntimeError("未找到 g++")

    OUTPUT.mkdir(exist_ok=True)
    environment = os.environ.copy()
    environment["PATH"] = str(ROOT / "tx") + os.pathsep + environment["PATH"]
    with tempfile.TemporaryDirectory(prefix="task_iocp_", dir=OUTPUT) as folder:
        work = Path(folder)
        (work / "tx_build").mkdir()
        program = work / "task_iocp_state_machine.exe"
        command = [
            compiler, "-std=c++23", "-O2", "-pthread", "-Isrc",
            str(SOURCE), "tx/libtxstdlib.a", "-Ltx/link",
            "-lwinhttp", "-lws2_32", "-ldnsapi", "-ladvapi32",
            "-lbcrypt", "-lcrypt32", "-lncrypt", "-lshell32",
            "-luser32", "-liconv", "-o", str(program),
        ]
        run(command, timeout=90)
        print(run([str(program)], timeout=15))
        print(run([str(program), "--shutdown-pending"], timeout=10))

        partial_program = work / "async_file_partial_write.exe"
        partial_command = [
            compiler, "-std=c++23", "-O2", "-pthread", "-Isrc",
            str(ROOT / "tests/stdlib/async_file_partial_write.cpp"),
            "tx/libtxstdlib.a", "-Ltx/link", "-lwinhttp", "-lws2_32",
            "-ldnsapi", "-ladvapi32", "-lbcrypt", "-lcrypt32", "-lncrypt",
            "-lshell32", "-luser32", "-liconv", "-o", str(partial_program),
        ]
        run(partial_command, timeout=90)
        print(run([str(partial_program)], timeout=15))

        compiler_path = ROOT / "tx/txc.exe"
        for name in ("async_file_behavior", "async_file_cancel_pending"):
            tx_program = work / f"{name}.exe"
            run([str(compiler_path), str(ROOT / "tests/stdlib" / f"{name}.tx"),
                 "-o", str(tx_program)], timeout=90, env=environment)
            run([str(tx_program)], timeout=15, cwd=work, env=environment)
            print(f"{name}: PASS")


if __name__ == "__main__":
    main()
