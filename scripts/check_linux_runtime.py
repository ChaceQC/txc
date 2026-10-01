"""Linux 进程、IPC、监视器、任务与异步文件的定向验收。"""

from pathlib import Path
import argparse
import os
import shutil
import subprocess
import sys
import tempfile

from check_platform import ROOT, TOOL_DIR, TXC, environment


def run(command, work, timeout):
    result = subprocess.run(
        [str(item) for item in command],
        cwd=work,
        env=environment(),
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        timeout=timeout,
        check=False,
    )
    if result.returncode:
        raise RuntimeError(
            f"{Path(str(command[0])).name} exit={result.returncode}\n"
            + result.stdout + result.stderr
        )
    if result.stdout.strip():
        print(result.stdout.strip())


def native_test(work):
    compiler = os.environ.get("CXX") or shutil.which("clang++-18") or shutil.which("clang++")
    if not compiler:
        raise RuntimeError("Linux 原生定向验收需要 clang++-18 或 CXX 指定的 C++23 编译器")
    program = work / "linux_process_runtime"
    libraries = (
        "icui18n", "icuuc", "icudata", "pcre2-8", "sodium", "argon2",
        "xml2", "pq", "curl", "ssl", "crypto", "cares", "z",
    )
    command = [
        compiler, "-std=c++23", "-O1", "-pthread", f"-I{ROOT / 'src'}",
        ROOT / "tests/platform/linux_process_runtime.cpp",
        TOOL_DIR / "libtxstdlib.a", f"-L{TOOL_DIR / 'lib'}",
        f"-Wl,-rpath,{TOOL_DIR / 'lib'}",
        *(f"-l{name}" for name in libraries), "-ldl", "-o", program,
    ]
    run(command, work, 180)
    run([program], work, 30)


def async_file_test(work):
    (work / "tx_build").mkdir(exist_ok=True)
    program = work / "async_file_behavior"
    run([TXC, ROOT / "tests/stdlib/async_file_behavior.tx", "-o", program], work, 180)
    run([program], work, 30)
    print("Linux async_file behavior passed")


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("groups", nargs="*", choices=("native", "async_file"))
    args = parser.parse_args()
    if not sys.platform.startswith("linux"):
        parser.error("此验收在 Linux 执行；Windows 可通过 WSL 运行")
    if not TXC.is_file() or not (TOOL_DIR / "libtxstdlib.a").is_file():
        parser.error("未找到 Linux 工具链，请设置 TXC_TOOL_DIR 为构建或解包目录")
    with tempfile.TemporaryDirectory(prefix="tx-linux-runtime-") as temporary:
        work = Path(temporary)
        for group in args.groups or ("native", "async_file"):
            if group == "native":
                native_test(work)
            else:
                async_file_test(work)


if __name__ == "__main__":
    main()
