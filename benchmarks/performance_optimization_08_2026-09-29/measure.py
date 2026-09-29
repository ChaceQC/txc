"""08 定向采样：先保存旧程序和 DLL，构建结束后交替测量。"""

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import statistics
import subprocess
import sys

root = Path(__file__).resolve().parents[2]
archive = Path(__file__).resolve().parent
work = root / "tx_build/performance_08"


def run(args):
    environment = os.environ.copy()
    environment["PATH"] = str(root / "tx") + os.pathsep + environment["PATH"]
    result = subprocess.run([str(arg) for arg in args], cwd=root, env=environment,
                            capture_output=True, encoding="utf-8", errors="strict", timeout=180)
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def identity():
    return {"head": run(["git", "rev-parse", "HEAD"]).strip(),
            "worktree": run(["git", "status", "--short"]),
            "source_sha256": digest(archive / "serde_paths.tx"),
            "toolchain_sha256": {name: digest(root / "tx" / name)
                                 for name in ["txc.exe", "libtxstdlib.a", "package.compat"]},
            "dll_sha256": {path.name: digest(path) for path in (root / "tx").glob("*.dll")}}


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    parser = argparse.ArgumentParser()
    parser.add_argument("--baseline", action="store_true")
    options = parser.parse_args()
    baseline = work / "baseline/serde_paths.exe"
    if options.baseline:
        baseline.parent.mkdir(parents=True, exist_ok=True)
        assert not baseline.exists(), "拒绝覆盖已有基线"
        run([root / "tx/txc.exe", archive / "serde_paths.tx", "-o", baseline])
        for path in (root / "tx").glob("*.dll"):
            shutil.copy2(path, baseline.parent / path.name)
        record = identity()
        record["program_sha256"] = digest(baseline)
        (archive / "baseline.json").write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
        print("已保存 07 基线程序及配套 DLL")
        return
    candidate = work / "serde_paths_08.exe"
    run([root / "tx/txc.exe", archive / "serde_paths.tx", "-o", candidate])
    reference = work / "serde_contract.exe"
    run(["g++", "-std=c++23", "-O3", "-DNDEBUG", "-Isrc", archive / "serde_contract.cpp",
         root / "tx/libtxstdlib.a", "-Ltx/link", "-lwinhttp", "-lws2_32", "-ladvapi32",
         "-lbcrypt", "-lshell32", "-luser32", "-liconv", "-o", reference])
    programs = {"07": baseline, "08": candidate, "cpp_contract": reference}
    results = {label: {} for label in programs}
    checksums = {}
    orders = []
    for round_index in range(6):
        order = list(programs) if round_index % 2 == 0 else list(reversed(programs))
        orders.append(order)
        for label in order:
            lines = run([programs[label]]).splitlines()
            assert len(lines) == 12, lines
            for index in range(0, len(lines), 3):
                name, elapsed, checksum = lines[index:index + 3]
                assert checksums.setdefault(name, checksum) == checksum, (name, checksum)
                if round_index:
                    results[label].setdefault(name, []).append(float(elapsed) / 1000)
    report = {"measured_at_utc": datetime.now(timezone.utc).isoformat(),
              "platform": platform.platform(), "processor": platform.processor(),
              "iterations": 5000, "warmups": 1, "rounds": 5, "orders": orders,
              "candidate": identity(),
              "cpp_contract": {"source_sha256": digest(archive / "serde_contract.cpp"),
                               "compiler": run(["g++", "--version"]).splitlines()[0],
                               "scope": "Same runtime schema and full JSON/CBOR validation; excludes TX handle/call boundary"},
              "program_sha256": {label: digest(path) for label, path in programs.items()},
              "results": {label: {name: {"samples_ms": values,
                          "median_ms": statistics.median(values), "checksum": checksums[name]}
                          for name, values in rows.items()} for label, rows in results.items()}}
    (archive / "samples.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    for label, rows in report["results"].items():
        print(label, {name: row["median_ms"] for name, row in rows.items()})


if __name__ == "__main__":
    main()
