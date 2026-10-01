"""Linux 工具包性能普查；串行采样，保留失败和原始输出，不覆盖历史数据。"""
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import os
import platform
import statistics
import subprocess
import sys
import time

root = Path(__file__).resolve().parents[2]
archive = Path(__file__).resolve().parent
work = root / "tx_build/linux_full_2026_10_01"
tool = root / "tx/linux"
results = {}


def save(name, value):
    (archive / name).write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def command(args, cwd=root, env=None, timeout=240):
    value = subprocess.run(list(map(str, args)), cwd=cwd, env=env, capture_output=True,
                           encoding="utf-8", errors="strict", timeout=timeout)
    if value.returncode:
        raise RuntimeError(f"{Path(args[0]).name} exit {value.returncode}: {value.stderr[-1600:]}")
    return value.stdout


def snapshot():
    paths = [p for folder in (root / "src", tool, archive) for p in folder.rglob("*")
             if p.is_file() and (folder != archive or p.suffix == ".py")]
    paths += [p for p in (root / "benchmarks").rglob("*") if p.suffix in (".tx", ".txh", ".cpp", ".hpp")]
    return {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(set(paths))}


def triples(output, divisor):
    lines = output.strip().splitlines()
    rows = [line.split() for line in lines] if all(len(line.split()) == 3 for line in lines) else [lines[i:i+3] for i in range(0, len(lines), 3)]
    values = {}
    for name, elapsed, checksum in rows:
        if name in values:
            raise ValueError("重复结果 " + name)
        values[name] = {"ms": float(elapsed) / divisor, "checksum": checksum}
    if not values:
        raise ValueError("缺少基准输出")
    return values


def measure(name, programs, divisors, cwd=root, env=None, expected=None):
    samples = {label: [] for label in programs}
    checks = expected
    for iteration in range(6):
        labels = list(programs)
        if iteration % 2:
            labels.reverse()
        for label in labels:
            started = time.perf_counter()
            output = command(programs[label], cwd=cwd, env=env)
            wall = (time.perf_counter() - started) * 1000
            values = triples(output, divisors[label])
            actual = {k: v["checksum"] for k, v in values.items()}
            if checks is None:
                checks = actual
            if actual != checks:
                raise ValueError(f"{name}/{label} 校验不一致: {actual} != {checks}")
            if iteration:
                samples[label].append({"wall_ms": wall, "cases": values, "stdout": output})
        print(f"PASS {name} {iteration}/5", flush=True)
    cases = {key: {label: statistics.median(s["cases"][key]["ms"] for s in rows)
                   for label, rows in samples.items()} for key in checks}
    return {"samples": samples, "medians_ms": cases, "checksums": checks,
            "validation": "fixed expected" if expected else "cross-language agreement" if len(programs) > 1 else "warmup consistency"}


def suite(name, source, cpp=None, divisor=1000, cwd=root, env=None, expected=None):
    try:
        binary = work / name
        start = time.perf_counter()
        command([tool / "txc", source, "-o", binary])
        compile_ms = (time.perf_counter() - start) * 1000
        programs = {"TX": [binary]}
        divisors = {"TX": divisor}
        if cpp:
            target = work / (name + "_cpp")
            command(["g++", "-std=c++23", "-O3", "-DNDEBUG", "-pthread", *cpp, "-o", target])
            programs["C++"] = [target]
            divisors["C++"] = 1 if name == "language" else divisor
        value = measure(name, programs, divisors, cwd, env, expected)
        value.update(source=str(source.relative_to(root)), compile_ms=compile_ms, status="passed")
        results[name] = value
    except Exception as error:
        results[name] = {"status": "failed", "error": str(error)}
        print(f"FAIL {name}: {error}", flush=True)
    save("results.json", results)


def main():
    if platform.system() != "Linux":
        raise RuntimeError("需要 Linux 原生环境")
    if (archive / "manifest.json").exists():
        raise RuntimeError("拒绝覆盖已有样本")
    work.mkdir(parents=True, exist_ok=True)
    before = snapshot()
    save("manifest.json", {"started_utc": datetime.now(timezone.utc).isoformat(),
         "head": command(["git", "rev-parse", "HEAD"]).strip(), "platform": platform.platform(),
         "cpu": command(["lscpu"]), "python": sys.version, "gcc": command(["g++", "--version"]),
         "toolchain": command([tool / "clang", "--version"]), "sha256": before,
         "policy": "existing Linux package; one warmup, five serial samples; /mnt/e filesystem; no Windows timing comparison"})
    language = root / "benchmarks/language_features"
    suite("language", language / "compare.tx", list(language.glob("*.cpp")))
    suite("library", root / "benchmarks/library_compare/compare.tx", divisor=1)
    audit = root / "benchmarks/performance_baseline_2026-09-29/audit"
    for name in ("compute", "features"):
        suite(name, audit / (name + ".tx"))
    suite("diverse", root / "benchmarks/diverse_performance.tx")
    special = {
        "borrowing": "call_borrowing.tx", "static_runtime": "static_runtime.tx",
        "parse_paths": "performance_optimization_06_2026-09-29/parse_paths.tx",
        "format_paths": "performance_optimization_07_2026-09-29/format_paths.tx",
        "serde_paths": "performance_optimization_08_2026-09-29/serde_paths.tx",
        "paths_09_11": "performance_optimization_09_11_2026-09-29/paths.tx",
        "paths_12_14": "performance_optimization_12_14_2026-09-29/paths.tx",
        "paths_15_16": "performance_optimization_15_16_2026-09-29/paths.tx",
        "file_io": "performance_optimization_15_16_2026-09-29/file_io.tx",
        "format_contract": "performance_completion_2026-09-30/format_contract.tx",
        "graph_contract": "performance_completion_2026-09-30/graph_contract.tx",
        "random_long": "library_compare/random_profile.tx"}
    for name, path in special.items():
        suite(name, root / "benchmarks" / path, divisor=1 if name == "random_long" else 1000)
    from fixtures import extended
    extended(suite, root, work, command)
    after = snapshot()
    save("completed.json", {"completed_utc": datetime.now(timezone.utc).isoformat(),
        "unchanged": before == after, "changed": [p for p in before.keys() | after.keys() if before.get(p) != after.get(p)],
        "passed": [k for k, v in results.items() if v["status"] == "passed"],
        "failed": [k for k, v in results.items() if v["status"] != "passed"]})


if __name__ == "__main__":
    main()
