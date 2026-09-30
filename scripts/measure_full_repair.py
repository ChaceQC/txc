"""同轮比较归档的旧程序与当前候选，不覆盖原始基准。"""
import argparse
from contextlib import contextmanager, nullcontext
from datetime import datetime, timezone
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import statistics
import shutil
import subprocess
import sys
import tempfile


root = Path(__file__).resolve().parents[1]
source = root / "benchmarks/stdlib_full_2026-09-30"
sys.path.insert(0, str(source))
spec = importlib.util.spec_from_file_location("repair_benchmark", source / "run.py")
benchmark = importlib.util.module_from_spec(spec)
spec.loader.exec_module(benchmark)
candidate = root / "tx_build/performance_full_repair"
previous = root / "tx_build/performance_retest_2026_09_30_full_rerun"
legacy = {
    "diverse": (root / "benchmarks/diverse_performance.tx", previous / "equivalence/diverse_tx.exe"),
    "language": (root / "benchmarks/language_features/compare.tx", root / "tx_build/language_features_tx.exe"),
    "compute": (root / "tx_build/perf_audit_20260927/compute.tx", previous / "equivalence/compute_tx.exe"),
    "features": (root / "tx_build/perf_audit_20260927/features.tx", previous / "equivalence/features_tx.exe"),
    "format": (root / "benchmarks/performance_completion_2026-09-30/format_contract.tx", previous / "contracts/format_tx.exe"),
    "graph": (root / "benchmarks/performance_completion_2026-09-30/graph_contract.cpp", previous / "contracts/graph_contract.exe"),
    "http": (root / "tx_build/perf_audit_20260927/http.tx", root / "tx_build/perf_audit_20260927/http_retest.exe"),
    "library": (root / "benchmarks/library_compare/compare.tx", root / "tx_build/library_compare_tx.exe"),
    "random_long": (root / "benchmarks/library_compare/random_profile.tx", previous / "random.exe"),
}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def build(program, source_path):
    command = [str(root / "tx/txc.exe"), str(source_path)]
    if source_path.suffix == ".cpp":
        command = ["g++", "-std=c++23", "-O3", "-finput-charset=UTF-8", "-fexec-charset=UTF-8",
                   "-Isrc", str(source_path), "tx/libtxstdlib.a", "-Ltx/link", "-lwinhttp",
                   "-lws2_32", "-ladvapi32", "-lbcrypt", "-lshell32", "-luser32", "-liconv", "-lpsapi"]
    result = subprocess.run([*command, "-o", str(candidate / (program + ".exe"))],
                            cwd=root, capture_output=True, encoding="utf-8", timeout=180)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)


def measure_legacy(command, environment, milliseconds=False, single_name=None):
    result = subprocess.run(list(map(str, command)), cwd=root, env=environment,
                            capture_output=True, encoding="utf-8", timeout=180, check=True)
    lines = result.stdout.splitlines()
    if single_name is not None and len(lines) == 2:
        lines.insert(0, single_name)
    rows = ([line.split() for line in lines] if all(len(line.split()) == 3 for line in lines)
            else [lines[index:index + 3] for index in range(0, len(lines), 3)])
    cases = {}
    for name, elapsed, checksum in rows:
        if name in cases or float(elapsed) < 0:
            raise ValueError("invalid legacy measurement")
        cases[name] = {"ms": float(elapsed) / (1 if milliseconds else 1000), "checksum": checksum}
    return {"cases": cases}


@contextmanager
def http_server():
    server = subprocess.Popen([sys.executable, "-X", "utf8", "-B",
        str(root / "tx_build/perf_audit_20260927/http_server.py")], cwd=root,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding="utf-8")
    try:
        if server.stdout.readline().strip() != "READY":
            raise RuntimeError("HTTP benchmark server did not start")
        yield {}
    finally:
        server.terminate()
        server.communicate(timeout=5)


def measure_group(group, rounds, environment, compile_program):
    is_legacy = group in legacy
    program = group if is_legacy else "database" if group in ("sqlite", "postgres") else group
    source_path, old = (legacy[group] if is_legacy else
                        (source / (program + ".tx"), benchmark.work / (program + ".exe")))
    if compile_program:
        build(program, source_path)
    programs = {"old": old, "new": candidate / (program + ".exe")}
    hashes = {label: digest(path) for label, path in programs.items()}
    fixtures = benchmark.work / "fixtures"
    with tempfile.TemporaryDirectory(prefix="tx-repair-") as temporary:
        temporary = Path(temporary)
        service = nullcontext({})
        if group == "network":
            service = benchmark.servers(fixtures)
        elif group == "http":
            service = http_server()
        elif group == "postgres":
            database = benchmark.load("repair_database", root / "scripts/check_db_postgres.py")
            service = database.temporary_server(temporary, database.server_root())
        with service as variables:
            env = {**environment, **variables, "BENCH_GROUP": group}
            env["PATH"] = str(root / "tx") + os.pathsep + env["PATH"]
            samples = {"old": [], "new": []}
            checksums = None
            for iteration in range(rounds + 1):
                order = ["old", "new"] if iteration % 2 == 0 else ["new", "old"]
                for variant in order:
                    folder = temporary / f"{iteration}_{variant}"
                    folder.mkdir()
                    if group == "network":
                        for fixture in fixtures.iterdir():
                            if fixture.suffix != ".key":
                                shutil.copy2(fixture, folder / fixture.name)
                    sample = (measure_legacy([programs[variant]], env, group in ("library", "random_long"),
                                             "random_long" if group == "random_long" else None) if is_legacy else
                              benchmark.measure([programs[variant]], folder, env, benchmark.groups[group]))
                    actual = {key: value["checksum"] for key, value in sample["cases"].items()}
                    if checksums is None:
                        checksums = actual
                    if actual != checksums:
                        raise ValueError("old/new checksums differ: " + group)
                    if iteration:
                        samples[variant].append(sample)
    if hashes != {label: digest(path) for label, path in programs.items()}:
        raise RuntimeError("program changed during measurement")
    for name in checksums:
        before = statistics.median(x["cases"][name]["ms"] for x in samples["old"])
        after = statistics.median(x["cases"][name]["ms"] for x in samples["new"])
        ratio = before / after if after else float("inf")
        print(f"{name}: {before:.3f} -> {after:.3f} ms ({ratio:.2f}x)", flush=True)
    return {**samples, "source": str(source_path.relative_to(root)), "source_sha256": digest(source_path),
            "program_sha256": hashes}


def measure_external(group, rounds, environment, compile_program):
    project = "mini-filesystem" if group == "mini" else "not-yet-on-stage"
    project_root = Path("E:/Project/problems") / project
    source_path = project_root / "src/solution_benchmark.tx"
    if compile_program:
        build(group, source_path)
    programs = {"old": root / "tx_build/language_comparison" / group / "tx.exe",
                "new": candidate / (group + ".exe")}
    names = (["fanout-2000", "moves-2000", "random-2000-1"] if group == "mini" else
             ["forced-increasing-max"])
    hashes = {label: digest(path) for label, path in programs.items()}
    samples = {label: [{"cases": {}} for _ in range(rounds)] for label in programs}
    inputs = {}
    for name in names:
        path = project_root / "testdata" / (name + ".in")
        data, expected = path.read_bytes(), path.with_suffix(".out").read_bytes()
        inputs[name] = {"input_sha256": digest(path), "output_sha256": digest(path.with_suffix(".out"))}
        normalize = bytes.splitlines if group == "mini" else bytes.split
        for iteration in range(rounds + 1):
            order = ["old", "new"] if iteration % 2 == 0 else ["new", "old"]
            for label in order:
                result = subprocess.run([str(programs[label])], input=data, capture_output=True,
                                        env=environment, timeout=30)
                if result.returncode or normalize(result.stdout) != normalize(expected):
                    raise RuntimeError("external program answer mismatch: " + name)
                if iteration:
                    samples[label][iteration - 1]["cases"][name] = {
                        "ms": float(result.stderr.strip()) / 1000, "checksum": inputs[name]["output_sha256"]}
        before = statistics.median(x["cases"][name]["ms"] for x in samples["old"])
        after = statistics.median(x["cases"][name]["ms"] for x in samples["new"])
        print(f"{name}: {before:.3f} -> {after:.3f} ms ({before / after:.2f}x)", flush=True)
    if hashes != {label: digest(path) for label, path in programs.items()}:
        raise RuntimeError("program changed during measurement")
    return {**samples, "program_sha256": hashes, "inputs": inputs,
            "source": str(source_path), "source_sha256": digest(source_path)}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--groups", nargs="+", default=["concurrency", "diagnostics", "sqlite"])
    parser.add_argument("--rounds", type=int, default=3)
    parser.add_argument("--name", required=True)
    parser.add_argument("--build", action="store_true")
    args = parser.parse_args()
    output = root / "benchmarks/performance_full_repair_2026-09-30" / (args.name + ".json")
    if output.exists():
        raise RuntimeError("refusing to overwrite measurements")
    environment = os.environ.copy()
    environment["PATH"] = str(root / "tx") + os.pathsep + environment["PATH"]
    candidate.mkdir(parents=True, exist_ok=True)
    if args.rounds < 1 or args.rounds > 7:
        raise ValueError("rounds must be 1..7")
    results = {"manifest": {"started_utc": datetime.now(timezone.utc).isoformat(),
        "warmups": 1, "rounds": args.rounds,
        "compiler_sha256": digest(root / "tx/txc.exe"),
        "stdlib_sha256": digest(root / "tx/libtxstdlib.a")}}
    for group in args.groups:
        if group not in {*legacy, "concurrency", "diagnostics", "sqlite", "postgres", "network", "mini", "stage"}:
            raise ValueError("unknown group: " + group)
        measure = measure_external if group in ("mini", "stage") else measure_group
        results[group] = measure(group, args.rounds, environment, args.build)
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(results, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    if (results["manifest"]["compiler_sha256"] != digest(root / "tx/txc.exe") or
            results["manifest"]["stdlib_sha256"] != digest(root / "tx/libtxstdlib.a")):
        raise RuntimeError("toolchain changed during measurement")
    results["manifest"]["completed_utc"] = datetime.now(timezone.utc).isoformat()
    output.write_text(json.dumps(results, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
