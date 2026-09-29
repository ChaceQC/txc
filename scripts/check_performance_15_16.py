"""只运行 15 项涉及的 bytes、编码、统计和转换边界，以及包兼容检查。"""
import hashlib
import json
import os
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / "tx_build/performance_15_16/checks"
archive = root / "benchmarks/performance_optimization_15_16_2026-09-29"


def run(*args):
    environment = os.environ.copy()
    environment["PATH"] = str(root / "tx") + os.pathsep + environment["PATH"]
    result = subprocess.run([str(arg) for arg in args], cwd=root, env=environment,
                            capture_output=True, encoding="utf-8", errors="strict", timeout=90)
    assert result.returncode == 0, (args, result.stdout, result.stderr)
    return result.stdout


def main():
    out.mkdir(parents=True, exist_ok=True)
    results = {}
    for case in ("performance_15_16/libraries", "statistics/basic",
                 "bytes_file_stream/behavior", "bytes_file_stream/encodings"):
        source = root / "tests" / (case + ".tx")
        executable = out / (case.replace("/", "_") + ".exe")
        run(root / "tx/txc.exe", source, "-o", executable)
        results[case] = {"output": run(executable), "sha256": hashlib.sha256(source.read_bytes()).hexdigest()}
        print("PASS", case, flush=True)
    executable = out / "hex_allocations.exe"
    run("g++", "-std=c++23", "-O2", "-Isrc", "-finput-charset=UTF-8", "-fexec-charset=UTF-8",
        root / "tests/performance_15_16/hex_allocations.cpp", root / "tx/libtxstdlib.a",
        "-Ltx/link", "-lwinhttp", "-lws2_32", "-ladvapi32", "-lbcrypt", "-lshell32",
        "-luser32", "-liconv", "-o", executable)
    results["hex_allocations"] = run(executable)
    print("PASS hex allocation and high-byte boundaries", flush=True)
    results["package_compatibility"] = run("python", "-X", "utf8", root / "scripts/check_package_compatibility.py")
    (archive / "checks.json").write_text(json.dumps(results, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print("PASS package compatibility")


if __name__ == "__main__":
    main()
