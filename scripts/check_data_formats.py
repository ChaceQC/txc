"""JSON/CSV 定向验证；只运行所选格式的行为、边界和静态诊断。"""

from pathlib import Path
import csv
import json
import os
import subprocess
import sys


root = Path(__file__).resolve().parents[1]
output = root / "tx_build"
sys.stdout.reconfigure(encoding="utf-8")


def run(command):
    environment = os.environ.copy()
    environment["PATH"] = str(root / "tx") + os.pathsep + environment["PATH"]
    result = subprocess.run([str(part) for part in command], cwd=root,
                            env=environment, capture_output=True, timeout=60)
    data = result.stdout + result.stderr
    try:
        text = data.decode("utf-8")
    except UnicodeDecodeError:
        # 兼容现有 Windows 子进程的代码页诊断，终端仍统一输出 UTF-8。
        text = data.decode("gb18030", errors="replace")
    if result.returncode:
        raise AssertionError(f"exit={result.returncode}\n{text}")
    return text.strip()


def main():
    output.mkdir(exist_ok=True)
    for group in sys.argv[1:] or ["json"]:
        if group not in ("json", "csv"):
            raise ValueError(group)
        cases = [f"tests/{group}/stream.tx", f"examples/{group}_stream.tx"]
        if group == "json":
            cases.insert(0, "tests/json/behavior.tx")
        for case in cases:
            program = output / f"{group}_{Path(case).stem}_check.exe"
            run([root / "tx/txc.exe", root / case, "-o", program])
            print(run([program]))
        program = output / f"{group}_native_check.exe"
        run(["g++", "-std=c++23", "-O2", "-finput-charset=UTF-8", "-fexec-charset=UTF-8",
             "-Isrc", root / f"tests/{group}/stream_native.cpp", root / "tx/libtxstdlib.a",
             "-Ltx/link", "-lwinhttp", "-lws2_32", "-ladvapi32", "-lbcrypt", "-lshell32",
             "-luser32", "-liconv", "-o", program])
        print(run([program]))
        source = root / f"tests/{group}/stream_type_error.tx"
        result = subprocess.run([str(root / "tx/txc.exe"), "check", str(source)],
                                cwd=root, capture_output=True, timeout=15)
        assert result.returncode != 0 and b"stream_type_error.tx:5:" in result.stderr
        print(f"{group.upper()}_STREAM_STATIC_DIAGNOSTIC_OK")
        if group == "json":
            actual = json.loads((output / "json_stream_check.json").read_text(encoding="utf-8"))
            assert actual == [{"count": 3, "name": "甲𝄞"}, None]
        else:
            with (output / "csv_stream_check.csv").open(encoding="utf-8-sig", newline="") as source:
                assert list(csv.reader(source, strict=True)) == [
                    ["name", "note", "empty"], ["中文𝄞", "一\r\n二,\"三\"", ""]]
        print(f"{group.upper()}_PYTHON_INTEROP_OK")


if __name__ == "__main__":
    main()
