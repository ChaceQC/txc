"""06 定向采样；TX 基线程序须在改动前编译并配对保存 DLL。"""

from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import platform
import statistics
import subprocess
import sys

root = Path(__file__).resolve().parents[2]
archive = Path(__file__).resolve().parent
work = root / "tx_build/performance_06"
sys.stdout.reconfigure(encoding="utf-8")


def run(args):
    return subprocess.run([str(arg) for arg in args], cwd=root, capture_output=True,
                          encoding="utf-8", errors="strict", check=True, timeout=180).stdout


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def samples(programs):
    results = {label: {} for label in programs}
    checksums = {}
    orders = []
    for round_index in range(6):
        order = list(programs) if round_index % 2 == 0 else list(reversed(programs))
        orders.append(order)
        for label in order:
            lines = run([programs[label]]).splitlines()
            assert len(lines) % 3 == 0
            for index in range(0, len(lines), 3):
                name, elapsed, checksum = lines[index:index + 3]
                key = (label, name)
                assert checksums.setdefault(key, checksum) == checksum
                if round_index:
                    results[label].setdefault(name, []).append(float(elapsed) / 1000)
    for label in programs:
        for name in results[label]:
            reference = next(iter(programs))
            assert checksums[(label, name)] == checksums[(reference, name)]
    return {"warmups": 1, "rounds": 5, "order": orders,
            "program_sha256": {name: digest(path) for name, path in programs.items()},
            "results": {label: {name: {"samples_ms": values,
                        "median_ms": statistics.median(values), "checksum": checksums[(label, name)]}
                        for name, values in items.items()} for label, items in results.items()}}


def main():
    work.mkdir(parents=True, exist_ok=True)
    old_source = work / "baseline_src"
    for name in ["stdlib/parse.hpp", "stdlib/parse.cpp", "stdlib/error.hpp", "common/error_kind.hpp"]:
        target = old_source / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(run(["git", "show", "d44609e:src/" + name]), encoding="utf-8")
    native = {}
    for label, include, definition in [
        ("05_full_native", old_source, []),
        ("06_full_native", root / "src", []),
        ("06_scalar_core", root / "src", ["-DSCALAR_CORE"]),
    ]:
        target = work / (label + ".exe")
        run(["g++", "-std=c++23", "-O3", "-DNDEBUG", *definition, "-I", include,
             archive / "parse_core.cpp", include / "stdlib/parse.cpp", "-o", target])
        native[label] = target
    candidate = work / "parse_paths_06.exe"
    run([root / "tx/txc.exe", archive / "parse_paths.tx", "-o", candidate])
    report = {
        "measured_at_utc": datetime.now(timezone.utc).isoformat(),
        "baseline_commit": run(["git", "rev-parse", "d44609e"]).strip(),
        "platform": platform.platform(), "processor": platform.processor(),
        "native_compiler": run(["g++", "--version"]).splitlines()[0],
        "native_flags": "-std=c++23 -O3 -DNDEBUG; scalar variant adds -DSCALAR_CORE; no LTO",
        "iterations": {"tx": 100000, "native": 1000000},
        "timing": "internal microseconds converted to ms; native full result is C++ operation_result",
        "source_sha256": {name: digest(archive / name) for name in ["parse_paths.tx", "parse_core.cpp"]},
        "toolchain_sha256": {name: digest(root / "tx" / name)
                            for name in ["txc.exe", "libtxstdlib.a", "package.compat"]},
        "dll_sha256": {label: {path.name: digest(path) for path in directory.glob("*.dll")}
                       for label, directory in [("05", root / "tx_build/parse_05_package"), ("06", root / "tx")]},
        "tx": samples({"05": root / "tx_build/parse_05_package/parse_paths.exe", "06": candidate}),
        "native": samples(native),
    }
    (archive / "samples.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    for group in ["tx", "native"]:
        for label, rows in report[group]["results"].items():
            print(label, {name: row["median_ms"] for name, row in rows.items()})


if __name__ == "__main__":
    main()
