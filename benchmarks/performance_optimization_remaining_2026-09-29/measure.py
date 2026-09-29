"""冻结本轮工作树产物，仅重测计划中仍有回退的四组负载。"""
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
work = root / "tx_build/performance_remaining"
sources = {
    "borrowing": root / "benchmarks/call_borrowing.tx",
    "diverse": root / "benchmarks/diverse_performance.tx",
    "paths": root / "benchmarks/performance_optimization_15_16_2026-09-29/paths.tx",
    "parse": root / "benchmarks/performance_optimization_06_2026-09-29/parse_paths.tx",
}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(*args):
    environment = os.environ.copy()
    environment["PATH"] = str(root / "tx") + os.pathsep + environment["PATH"]
    started = time.perf_counter()
    process = subprocess.run([str(arg) for arg in args], cwd=root, env=environment,
                             capture_output=True, encoding="utf-8", errors="strict", timeout=180)
    if process.returncode:
        raise RuntimeError(f"{args}\n{process.stdout}\n{process.stderr}")
    return process.stdout, (time.perf_counter() - started) * 1000


def save(name, data):
    target = archive / name
    if target.exists():
        raise FileExistsError(target)
    target.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def compile_programs(package, destination):
    destination.mkdir(parents=True, exist_ok=True)
    costs = {}
    for name, source in sources.items():
        _, costs[name] = run(package / "txc.exe", source, "-o", destination / f"{name}.exe")
    for dependency in package.glob("*.dll"):
        shutil.copy2(dependency, destination / dependency.name)
    return costs


def baseline():
    destination = work / "baseline"
    destination.mkdir(parents=True, exist_ok=False)
    package = destination / "tx"
    shutil.copytree(root / "tx", package)
    costs = compile_programs(package, destination)
    files = [*destination.rglob("*"), *sources.values()]
    save("baseline.json", {
        "head": run("git", "rev-parse", "HEAD")[0].strip(),
        "initial_diff": run("git", "diff", "--stat")[0],
        "platform": platform.platform(),
        "gcc": run("g++", "--version")[0].splitlines()[0],
        "clang": run(package / "clang.exe", "--version")[0].splitlines()[0],
        "compile_wall_ms": costs,
        "sha256": {str(path.relative_to(root)): digest(path) for path in files if path.is_file()},
    })
    print("Baseline package, DLLs and four workloads frozen", flush=True)


def read_program(program):
    output, wall_ms = run(program)
    lines = output.splitlines()
    if all(len(line.split()) == 3 for line in lines):
        lines = [part for line in lines for part in line.split()]
    assert lines and len(lines) % 3 == 0, output
    cases = {lines[i]: {"ms": float(lines[i + 1]) / 1000, "checksum": int(lines[i + 2])}
             for i in range(0, len(lines), 3)}
    return {"wall_ms": wall_ms, "cases": cases}


def final(filename):
    record = json.loads((archive / "baseline.json").read_text(encoding="utf-8"))
    for name, expected in record["sha256"].items():
        assert digest(root / name) == expected, name
    costs = compile_programs(root / "tx", work / "candidate")
    programs = {f"{name}_{label}": work / directory / f"{name}.exe"
                for name in sources for label, directory in (("old", "baseline"), ("new", "candidate"))}
    expected = {label: {name: data["checksum"] for name, data in read_program(program)["cases"].items()}
                for label, program in programs.items()}
    for name in sources:
        assert expected[f"{name}_old"] == expected[f"{name}_new"], name
    samples = {label: [] for label in programs}
    orders = []
    for index in range(5):
        order = list(programs) if index % 2 == 0 else list(reversed(programs))
        orders.append(order)
        for label in order:
            measured = read_program(programs[label])
            assert {name: data["checksum"] for name, data in measured["cases"].items()} == expected[label]
            samples[label].append(measured)
    medians = {label: {name: statistics.median(sample["cases"][name]["ms"] for sample in values)
                       for name in expected[label]} for label, values in samples.items()}
    files = [*programs.values(), root / "tx/txc.exe", root / "tx/libtxstdlib.a", root / "tx/package.compat",
             *(work / "candidate").glob("*.dll")]
    save(filename, {"warmups": 1, "rounds": 5, "orders": orders, "samples": samples,
                    "checksums": expected, "medians_ms": medians, "compile_wall_ms": costs,
                    "sha256": {str(path.relative_to(root)): digest(path) for path in files}})
    print(json.dumps(medians, indent=2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=["baseline", "final"])
    parser.add_argument("--output", default="samples.json")
    arguments = parser.parse_args()
    baseline() if arguments.mode == "baseline" else final(arguments.output)
