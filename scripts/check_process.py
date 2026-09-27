"""第七节定向验证；不读取或输出真实环境值，程序均设置宿主超时。"""

from pathlib import Path
import subprocess
import sys
import tempfile
import time


root = Path(__file__).resolve().parents[1]
output = root / "tx_build"
sys.stdout.reconfigure(encoding="utf-8")


def run(command, **kwargs):
    result = subprocess.run(command, capture_output=True, timeout=45, **kwargs)
    def decode(data):
        try:
            return data.decode("utf-8")
        except UnicodeDecodeError:
            # 旧运行时的重定向中文诊断可能使用系统代码页，统一转为 UTF-8 输出。
            return data.decode("gb18030", errors="replace")
    if result.returncode:
        time.sleep(2.2)
        raise AssertionError(f"exit={result.returncode}\n" +
                             decode(result.stdout) + decode(result.stderr))
    return decode(result.stdout).strip()


def main():
    output.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="子进程 中文 空格_", dir=output) as folder:
        work = Path(folder)
        assert work.resolve().is_relative_to(output.resolve())
        helper = work / "child helper.exe"
        run(["g++", "-std=c++23", "-O2", "-municode", "-static",
             str(root / "tests/stdlib/process_child.cpp"), "-o", str(helper)])
        groups = sys.argv[1:] or ["lifecycle", "pipes", "contracts", "resources",
                                  "filesystem", "diagnostics"]
        for group in groups:
            if group == "diagnostics":
                source = root / "tests/errors/process_pipe_type.tx"
                result = subprocess.run([str(root / "tx/txc.exe"), "check", str(source)],
                                        capture_output=True, timeout=15)
                assert result.returncode != 0
                assert b"process_pipe_type.tx:5:" in result.stderr
                print("PROCESS_STATIC_DIAGNOSTIC_OK")
                continue
            if group in ("resources", "filesystem"):
                name = "process_resources" if group == "resources" else "filesystem_boundaries"
                program = output / f"{name}.exe"
                command = ["g++", "-std=c++23", "-O2", "-Isrc",
                           str(root / f"tests/stdlib/{name}.cpp"),
                           str(root / "tx/libtxstdlib.a"), "-Ltx/link",
                           "-lwinhttp", "-lws2_32", "-ladvapi32", "-lbcrypt",
                           "-lshell32", "-luser32", "-liconv", "-o", str(program)]
                if group == "resources":
                    command.append("-municode")
                run(command, cwd=root)
                startup = subprocess.STARTUPINFO()
                startup.dwFlags = subprocess.STARTF_USESHOWWINDOW
                startup.wShowWindow = subprocess.SW_HIDE
                print(run([str(program), str(helper)], cwd=work, startupinfo=startup,
                          creationflags=subprocess.CREATE_NEW_CONSOLE))
                continue
            name = f"process_{group}"
            program = output / f"{name}.exe"
            run([str(root / "tx/txc.exe"), str(root / f"tests/stdlib/{name}.tx"),
                 "-o", str(program)])
            print(run([str(program), str(helper)], cwd=work))


if __name__ == "__main__":
    main()
