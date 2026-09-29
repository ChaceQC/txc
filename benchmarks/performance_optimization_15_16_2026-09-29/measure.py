"""冻结旧包；定向预查；最终一次同轮汇总。所有输出均为 UTF-8。"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import statistics
import subprocess
import time

root = Path(__file__).resolve().parents[2]
archive = Path(__file__).resolve().parent
out = root / "tx_build/performance_15_16"
sources = {
    "paths": archive / "paths.tx",
    "diverse": root / "benchmarks/diverse_performance.tx",
    "language": root / "benchmarks/language_features/compare.tx",
    "features": root / "benchmarks/performance_baseline_2026-09-29/audit/features.tx",
    "compute": root / "benchmarks/performance_baseline_2026-09-29/audit/compute.tx",
    "borrowing": root / "benchmarks/call_borrowing.tx",
}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(name, value):
    target = archive / name
    if target.exists():
        version = 1
        previous = target.with_stem(target.stem + f"_previous_{version}")
        while previous.exists():
            version += 1
            previous = target.with_stem(target.stem + f"_previous_{version}")
        shutil.copy2(target, previous)
    target.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def run(*args):
    environment = os.environ.copy()
    environment["PATH"] = str(root / "tx") + os.pathsep + environment["PATH"]
    started = time.perf_counter()
    result = subprocess.run([str(a) for a in args], cwd=root, env=environment,
                            capture_output=True, encoding="utf-8", errors="strict", timeout=180)
    assert result.returncode == 0, (args, result.stdout, result.stderr)
    return result.stdout, (time.perf_counter() - started) * 1000


def compile_programs(package, destination):
    destination.mkdir(parents=True, exist_ok=True)
    costs = {}
    for name, source in sources.items():
        _, elapsed = run(package / "txc.exe", source, "-o", destination / (name + ".exe"))
        costs[name] = elapsed
    return costs


def read_program(program):
    text, wall_ms = run(*(program if isinstance(program, list) else [program]))
    lines = text.splitlines()
    if lines and all(len(line.split()) == 3 for line in lines):
        lines = [item for line in lines for item in line.split()]
    assert len(lines) % 3 == 0, text
    result = {}
    for index in range(0, len(lines), 3):
        name = lines[index]
        assert name not in result, name
        result[name] = {"ms": float(lines[index + 1]) / 1000, "checksum": int(lines[index + 2])}
    return {"wall_ms": wall_ms, "cases": result}


def sample(programs):
    expected = {}
    for label, program in programs.items():
        expected[label] = {k: v["checksum"] for k, v in read_program(program)["cases"].items()}
    samples = {label: [] for label in programs}
    orders = []
    for round_index in range(5):
        order = list(programs) if round_index % 2 == 0 else list(reversed(programs))
        orders.append(order)
        for label in order:
            measured = read_program(programs[label])
            assert {k: v["checksum"] for k, v in measured["cases"].items()} == expected[label], label
            samples[label].append(measured)
    medians = {label: {name: statistics.median(item["cases"][name]["ms"] for item in values)
                       for name in expected[label]} for label, values in samples.items()}
    return {"warmups": 1, "rounds": 5, "orders": orders, "samples": samples,
            "medians_ms": medians, "checksums": expected,
            "sha256": {str((p[0] if isinstance(p, list) else p).relative_to(root)):
                       digest(p[0] if isinstance(p, list) else p) for p in programs.values()}}


def baseline():
    destination = out / "baseline"
    destination.mkdir(parents=True, exist_ok=False)
    package = destination / "tx"
    shutil.copytree(root / "tx", package)
    costs = compile_programs(package, destination)
    files = [*package.rglob("*"), *destination.glob("*.exe"), *sources.values()]
    save("baseline.json", {
        "head": run("git", "rev-parse", "HEAD")[0].strip(),
        "initial_diff": run("git", "diff", "--stat")[0],
        "platform": platform.platform(),
        "gcc": run("g++", "--version")[0].splitlines()[0],
        "clang": run(package / "clang.exe", "--version")[0].splitlines()[0],
        "compile_wall_ms": costs,
        "package_bytes": sum(p.stat().st_size for p in package.rglob("*") if p.is_file()),
        "sha256": {str(p.relative_to(root)): digest(p) for p in files if p.is_file()},
    })
    result = sample({"baseline_paths": destination / "paths.exe"})
    save("preflight.json", result)
    print(json.dumps(result["medians_ms"], indent=2))


def final():
    baseline_record = json.loads((archive / "baseline.json").read_text(encoding="utf-8"))
    for name, checksum in baseline_record["sha256"].items():
        assert digest(root / name) == checksum, name
    costs = compile_programs(root / "tx", out / "candidate")
    programs = {f"{name}_{label}": out / folder / (name + ".exe")
                for name in sources for label, folder in (("old", "baseline"), ("new", "candidate"))}
    for label, library in (("old", out / "baseline/tx/libtxstdlib.a"),
                           ("new", root / "tx/libtxstdlib.a")):
        executable = out / (label + "_core.exe")
        _, costs[label + "_core"] = run("g++", "-std=c++23", "-O3", "-Isrc",
            "-finput-charset=UTF-8", "-fexec-charset=UTF-8", archive / "core.cpp", library,
            "-Ltx/link", "-lwinhttp", "-lws2_32", "-ladvapi32", "-lbcrypt", "-lshell32",
            "-luser32", "-liconv", "-o", executable)
        programs["core_" + label] = executable
    programs["cpp_reference"] = [out / "new_core.exe", "reference"]
    result = sample(programs)
    for name in sources:
        assert result["checksums"][name + "_old"] == result["checksums"][name + "_new"], name
    for label in ("core_old", "core_new", "cpp_reference"):
        for name, checksum in result["checksums"][label].items():
            assert result["checksums"]["paths_new"][name] == checksum, (label, name)
    result["compile_wall_ms"] = costs
    result["package_bytes"] = sum(p.stat().st_size for p in (root / "tx").rglob("*") if p.is_file())
    result["sha256"].update({str(p.relative_to(root)): digest(p) for p in
                            [root / "tx/txc.exe", root / "tx/libtxstdlib.a", root / "tx/package.compat"]})
    save("samples.json", result)
    print(json.dumps(result["medians_ms"], indent=2))


def focused():
    destination = out / "refined"
    destination.mkdir(parents=True, exist_ok=True)
    run(root / "tx/txc.exe", sources["paths"], "-o", destination / "paths.exe")
    result = sample({"old": out / "baseline/paths.exe", "new": destination / "paths.exe"})
    assert result["checksums"]["old"] == result["checksums"]["new"]
    save("refined.json", result)
    print(json.dumps(result["medians_ms"], indent=2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=["baseline", "final", "focused"])
    globals()[parser.parse_args().mode]()
