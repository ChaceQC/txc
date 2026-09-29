"""07 定向基线及交替采样；--baseline 必须在修改工具链之前运行。"""

from datetime import datetime, timezone
import hashlib
import json
import argparse
from pathlib import Path
import platform
import shutil
import statistics
import subprocess
import sys

root = Path(__file__).resolve().parents[2]
archive = Path(__file__).resolve().parent
work = root / "tx_build/performance_07"


def run(args):
    return subprocess.run([str(arg) for arg in args], cwd=root, capture_output=True,
                          encoding="utf-8", errors="strict", check=True, timeout=180).stdout


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def identity():
    return {"head": run(["git", "rev-parse", "HEAD"]).strip(),
            "source_sha256": digest(archive / "format_paths.tx"),
            "toolchain_sha256": {name: digest(root / "tx" / name)
                                 for name in ["txc.exe", "libtxstdlib.a", "package.compat"]},
            "dll_sha256": {path.name: digest(path) for path in (root / "tx").glob("*.dll")}}


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    parser = argparse.ArgumentParser()
    parser.add_argument("--baseline", action="store_true")
    parser.add_argument("--output", default="samples_final.json")
    options = parser.parse_args()
    baseline = work / "baseline"
    if options.baseline:
        baseline.mkdir(parents=True, exist_ok=True)
        target = baseline / "format_paths.exe"
        assert not target.exists(), "基线已存在，拒绝覆盖"
        run([root / "tx/txc.exe", archive / "format_paths.tx", "-o", target])
        for path in (root / "tx").glob("*.dll"):
            shutil.copy2(path, baseline / path.name)
        record = identity()
        record["program_sha256"] = digest(target)
        (archive / "baseline.json").write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
        print("已保存 06 基线程序、DLL 及标识")
        return
    candidate = work / "format_paths_07.exe"
    run([root / "tx/txc.exe", archive / "format_paths.tx", "-o", candidate])
    reference = work / "format_reference.exe"
    run(["g++", "-std=c++23", "-O3", "-DNDEBUG", archive / "format_reference.cpp", "-o", reference])
    programs = {"06": baseline / "format_paths.exe", "07": candidate, "cpp_reference": reference}
    results = {label: {} for label in programs}
    checksums = {}
    orders = []
    for round_index in range(6):
        order = list(programs) if round_index % 2 == 0 else list(reversed(programs))
        orders.append(order)
        for label in order:
            lines = run([programs[label]]).splitlines()
            assert len(lines) == 9, lines
            for index in range(0, len(lines), 3):
                name, elapsed, checksum = lines[index:index + 3]
                assert checksums.setdefault(name, checksum) == checksum
                if round_index:
                    results[label].setdefault(name, []).append(float(elapsed) / 1000)
    report = {"measured_at_utc": datetime.now(timezone.utc).isoformat(),
              "platform": platform.platform(), "processor": platform.processor(),
              "iterations": 50000, "warmups": 1, "rounds": 5, "orders": orders,
              "candidate": identity(),
              "cpp_reference": {"scope": "fixed ASCII algorithm only, not equivalent public format API",
                                "source_sha256": digest(archive / "format_reference.cpp"),
                                "compiler": run(["g++", "--version"]).splitlines()[0],
                                "flags": "-std=c++23 -O3 -DNDEBUG"},
              "program_sha256": {label: digest(path) for label, path in programs.items()},
              "results": {label: {name: {"samples_ms": values,
                          "median_ms": statistics.median(values), "checksum": checksums[name]}
                          for name, values in rows.items()} for label, rows in results.items()}}
    (archive / options.output).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    for label, rows in report["results"].items():
        print(label, {name: row["median_ms"] for name, row in rows.items()})


if __name__ == "__main__":
    main()
