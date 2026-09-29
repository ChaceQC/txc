"""串行复测当前工作树，保留历史数据及本轮原始样本。"""
from datetime import datetime, timezone
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import statistics
import subprocess
import sys

root = Path(__file__).resolve().parents[2]
archive = Path(__file__).resolve().parent
work = root / "tx_build/performance_retest_2026_09_30"
os.environ["PYTHONUTF8"] = "1"
os.environ["PATH"] = str(root / "tx") + os.pathsep + os.environ["PATH"]


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, root / path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def save(name, value):
    (archive / name).write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def run(*args, timeout=1800):
    result = subprocess.run([str(arg) for arg in args], cwd=root, capture_output=True,
                            encoding="utf-8", errors="strict", timeout=timeout)
    if result.returncode:
        raise RuntimeError(f"{args}: {result.stdout[-2000:]}\n{result.stderr[-2000:]}")
    return result.stdout


def compile_tx(source, target):
    run(root / "tx/txc.exe", source, "-o", target)


def snapshot():
    paths = [*root.glob("src/**/*.cpp"), *root.glob("src/**/*.hpp"),
             *root.glob("tx/stdlib/**/*.txh"), *root.glob("tx/*.dll"),
             root / "tx/txc.exe", root / "tx/libtxstdlib.a", root / "tx/package.compat"]
    for pattern in ("*.tx", "*.cpp", "*.hpp", "*.py", "*.java", "*.ps1"):
        paths.extend(root.glob("benchmarks/**/" + pattern))
        paths.extend(root.glob("scripts/" + pattern))
    return {str(path.relative_to(root)): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in sorted(paths)}


def language():
    command = ". ./benchmarks/language_features/run_compare.ps1 -Rounds 7; "
    command += "'RAW_JSON ' + (@{TX=$tx_samples; CPP=$cpp_samples; Python=$python_samples; Java=$java_samples; checksums=$expected} | ConvertTo-Json -Depth 8 -Compress)"
    output = run("pwsh", "-NoProfile", "-Command", command)
    (archive / "language_features.log").write_text(output, encoding="utf-8")
    raw = next(line[9:] for line in output.splitlines() if line.startswith("RAW_JSON "))
    save("language_features.json", json.loads(raw))


def audit():
    module = load("retest", "tx_build/performance_retest_20260927.py")
    module.OUTPUT = archive / "performance_retest.json"
    for name in ("compute", "features", "system", "http", "ws"):
        compile_tx(module.AUDIT / f"{name}.tx", module.AUDIT / f"{name}_retest.exe")
    compile_tx(root / "benchmarks/library_compare/compare.tx", root / "tx_build/library_compare_tx.exe")
    cmake = Path(r"D:\CLion 2024.3.4\bin\cmake\win\x64\bin\cmake.exe")
    run(cmake, "-S", "benchmarks/library_compare", "-B", "tx_build/library_compare_cmake",
        "-G", "Ninja", "-DCMAKE_CXX_COMPILER=g++", "-DCMAKE_BUILD_TYPE=Release")
    run(cmake, "--build", "tx_build/library_compare_cmake")
    for suite in ("library", "audit", "network", "external"):
        if suite == "external":
            for key, project in (("mini", "mini-filesystem"), ("stage", "not-yet-on-stage")):
                source = Path("E:/Project/problems") / project / "src"
                binary = root / "tx_build/language_comparison" / key
                compile_tx(source / "solution_benchmark.tx", binary / "tx.exe")
                compile_tx(source / "solution.tx", binary / "solution.exe")
                for path in sorted((source.parent / "testdata").glob("*.in")):
                    result = subprocess.run([str(binary / "solution.exe")], input=path.read_bytes(),
                                            capture_output=True, timeout=30)
                    expected = path.with_suffix(".out").read_bytes()
                    normalize = bytes.splitlines if key == "mini" else bytes.split
                    assert result.returncode == 0 and normalize(result.stdout) == normalize(expected), path
                print(f"PASS {project}: full official data", flush=True)
        sys.argv = ["retest", suite]
        print(f"START {suite}", flush=True)
        module.main()


def special():
    reader = load("reader", "scripts/compare_performance_programs.py")
    sources = {
        "borrowing": "benchmarks/call_borrowing.tx",
        "static_runtime": "benchmarks/static_runtime.tx",
        "parse_paths": "benchmarks/performance_optimization_06_2026-09-29/parse_paths.tx",
        "format_paths": "benchmarks/performance_optimization_07_2026-09-29/format_paths.tx",
        "serde_paths": "benchmarks/performance_optimization_08_2026-09-29/serde_paths.tx",
        "paths_09_11": "benchmarks/performance_optimization_09_11_2026-09-29/paths.tx",
        "paths_12_14": "benchmarks/performance_optimization_12_14_2026-09-29/paths.tx",
        "paths_15_16": "benchmarks/performance_optimization_15_16_2026-09-29/paths.tx",
        "file_io": "benchmarks/performance_optimization_15_16_2026-09-29/file_io.tx",
    }
    results = {}
    for name, source in sources.items():
        target = work / f"{name}.exe"
        compile_tx(root / source, target)
        expected = reader.read_program(target)
        samples = []
        for _ in range(3 if name == "static_runtime" else 7):
            measured = reader.read_program(target)
            reader.check(expected, measured)
            samples.append(measured)
        results[name] = {"source": source, "warmups": 1, "samples": samples,
                         "medians_ms": {key: statistics.median(s[key][0] for s in samples) for key in expected}}
        save("special.json", results)
        print(f"PASS special {name}: {len(expected)} cases", flush=True)
    target = work / "random.exe"
    compile_tx(root / "benchmarks/library_compare/random_profile.tx", target)
    programs = {"TX": [target], "C++": [root / "tx_build/library_compare_cmake/library_compare_cpp.exe", "random-long"]}
    samples = {key: [] for key in programs}
    for index in range(8):
        for label in (list(programs) if index % 2 == 0 else list(reversed(programs))):
            lines = run(*programs[label]).splitlines()
            assert int(lines[-1]) == 2500067985, lines
            if index:
                samples[label].append(float(lines[-2]))
    results["random_long"] = {"warmups": 1, "samples_ms": samples, "checksum": 2500067985}
    save("special.json", results)


def main():
    work.mkdir(parents=True, exist_ok=True)
    before = snapshot()
    save("manifest.json", {"started_utc": datetime.now(timezone.utc).isoformat(),
                           "head": run("git", "rev-parse", "HEAD").strip(),
                           "initial_status": run("git", "status", "--short"), "sha256": before})
    phases = [("language", language), ("audit", audit), ("special", special)]
    for name, action in phases:
        print(f"START {name}", flush=True)
        action()
    diverse = load("diverse", "scripts/run_diverse_performance.py")
    diverse.output = archive / "diverse.json"
    print("START diverse", flush=True)
    diverse.main()
    equivalent = load("equivalent", "scripts/run_performance_equivalence.py")
    equivalent.work = work / "equivalence"
    sys.argv = ["equivalent", "--rounds", "5"]
    print("START equivalent", flush=True)
    equivalent.main()
    shutil.copy2(equivalent.work / "results_complete.json", archive / "equivalence.json")
    assert snapshot() == before, "Source or toolchain changed during measurement"
    save("completed.json", {"completed_utc": datetime.now(timezone.utc).isoformat(),
                            "source_and_toolchain_unchanged": True})
    print("ALL PERFORMANCE SUITES COMPLETE", flush=True)


if __name__ == "__main__":
    main()
