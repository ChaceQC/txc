"""同轮比较校准后的综合负载、统计和堆，并保留原始样本。"""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import statistics
import subprocess


root = Path(__file__).resolve().parent.parent
archive = root / "benchmarks/performance_baseline_2026-09-29"
work = root / "tx_build/performance_equivalence"
compiler = root / "tx/txc.exe"
library = root / "tx/libtxstdlib.a"
clang = root / "tx/clang.exe"
diverse_tx = root / "benchmarks/diverse_performance.tx"
diverse_cpp = root / "benchmarks/diverse_performance.cpp"
parse_cpp = root / "benchmarks/performance_equivalence/parse_contract.cpp"
parse_header = root / "benchmarks/performance_equivalence/parse_contract.hpp"
serde_cpp = root / "benchmarks/performance_equivalence/serde_payload.cpp"
serde_header = root / "benchmarks/performance_equivalence/serde_payload.hpp"
feature_cpp = root / "benchmarks/performance_equivalence/feature_contracts.cpp"
features_tx = archive / "audit/features.tx"
compute_tx = archive / "audit/compute.tx"
baseline_samples = archive / "samples/diverse_performance_results_20260929.json"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def run(command: list[str]) -> str:
    result = subprocess.run(command, cwd=root, capture_output=True,
                            encoding="utf-8", errors="replace", timeout=180,
                            check=False)
    if result.returncode:
        raise RuntimeError(f"{command[0]} exit {result.returncode}: {result.stderr[-1000:]}")
    return result.stdout


def triples(command: list[str]) -> dict[str, tuple[float, str]]:
    lines = run(command).splitlines()
    if len(lines) % 3:
        raise ValueError(f"基准输出不是三行一组：{command[0]}")
    result = {}
    for index in range(0, len(lines), 3):
        name = lines[index]
        if name in result:
            raise ValueError(f"重复基准项目：{name}")
        result[name] = (float(lines[index + 1]) / 1000, lines[index + 2])
    return result


def clang_flags() -> list[str]:
    gcc_include = Path(run(["g++", "-print-file-name=include"]).strip()).resolve()
    gcc_root = gcc_include.parent
    mingw_root = (gcc_root / "../../../..").resolve()
    cpp_include = gcc_root / "include/c++"
    mingw_lib = mingw_root / "x86_64-w64-mingw32/lib"
    return [str(clang), "-target", "x86_64-w64-windows-gnu", "-std=c++23",
            "-O3", "-DNDEBUG", "-isystem", str(cpp_include),
            "-isystem", str(cpp_include / "x86_64-w64-mingw32"),
            "-isystem", str(cpp_include / "backward"),
            "-isystem", str(gcc_include),
            "-isystem", str(mingw_root / "x86_64-w64-mingw32/include"),
            "-B", str(gcc_root), "-B", str(mingw_lib),
            "-L", str(gcc_root), "-L", str(mingw_lib),
            "-L", str(mingw_root / "lib")]


def build() -> dict[str, Path]:
    work.mkdir(parents=True, exist_ok=True)
    programs = {
        "diverse_tx": work / "diverse_tx.exe",
        "diverse_cpp": work / "diverse_cpp.exe",
        "diverse_cpp_clang": work / "diverse_cpp_clang.exe",
        "features_tx": work / "features_tx.exe",
        "compute_tx": work / "compute_tx.exe",
        "feature_cpp": work / "feature_cpp.exe",
        "feature_cpp_clang": work / "feature_cpp_clang.exe",
    }
    for source, program in ((diverse_tx, programs["diverse_tx"]),
                            (features_tx, programs["features_tx"]),
                            (compute_tx, programs["compute_tx"])):
        run([str(compiler), str(source), "-o", str(program)])
    flags = ["g++", "-std=c++23", "-O3", "-DNDEBUG"]
    run([*flags, str(diverse_cpp), str(parse_cpp), str(serde_cpp),
         "-o", str(programs["diverse_cpp"])])
    run([*flags, str(feature_cpp), "-o", str(programs["feature_cpp"])])
    clang_command = clang_flags()
    run([*clang_command, str(diverse_cpp), str(parse_cpp), str(serde_cpp),
         "-lstdc++", "-o", str(programs["diverse_cpp_clang"])])
    run([*clang_command, str(feature_cpp), "-lstdc++",
         "-o", str(programs["feature_cpp_clang"])])
    run([str(programs["diverse_cpp"]), "--contract-check"])
    run([str(programs["feature_cpp"]), "--contract-check"])
    run([str(programs["diverse_cpp_clang"]), "--contract-check"])
    run([str(programs["feature_cpp_clang"]), "--contract-check"])
    return programs


def compare(left: Path, right: Path, names: dict[str, str], rounds: int) -> dict:
    commands = {"TX": [str(left)], "C++": [str(right)]}

    def read(label: str) -> dict[str, float]:
        result = triples(commands[label])
        if not set(names).issubset(result):
            raise ValueError(f"{label} 缺少目标项目")
        if any(result[name][1] != checksum for name, checksum in names.items()):
            raise ValueError(f"{label} 的校验值与固定输入不符")
        return {name: result[name][0] for name in names}

    for label in commands:
        read(label)
    samples = {label: {name: [] for name in names} for label in commands}
    order_by_round = []
    for index in range(rounds):
        order = ("TX", "C++") if index % 2 == 0 else ("C++", "TX")
        order_by_round.append(order)
        for label in order:
            measurement = read(label)
            for name, elapsed in measurement.items():
                samples[label][name].append(elapsed)
    return {
        "warmups": 1, "rounds": rounds, "order_by_round": order_by_round,
        "timing": "program internal microseconds converted to milliseconds",
        "cases": {name: {
            "checksum": checksum,
            "TX_samples_ms": samples["TX"][name],
            "CPP_samples_ms": samples["C++"][name],
            "TX_median_ms": statistics.median(samples["TX"][name]),
            "CPP_median_ms": statistics.median(samples["C++"][name]),
        } for name, checksum in names.items()},
    }


def parse_core(program: Path, rounds: int) -> dict:
    command = [str(program), "--parse-core"]
    expected = {"parse_core_valid": "100000", "parse_core_invalid": "100000"}

    def read() -> dict[str, float]:
        result = triples(command)
        if set(result) != set(expected) or any(result[name][1] != checksum
                                               for name, checksum in expected.items()):
            raise ValueError("纯解析核心校验值错误")
        return {name: result[name][0] for name in expected}

    read()
    samples = [read() for _ in range(rounds)]
    return {name: {"checksum": checksum,
                   "samples_ms": [item[name] for item in samples],
                   "median_ms": statistics.median(item[name] for item in samples)}
            for name, checksum in expected.items()}


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--rounds", type=int, choices=(5, 6, 7), default=5)
    args = parser.parse_args()
    baseline = json.loads(baseline_samples.read_text(encoding="utf-8"))
    diverse_checksums = {
        name: str(values["TX"]["checksum"])
        for name, values in baseline["diverse"]["cases"].items()
    }
    sources = (compiler, library, clang, diverse_tx, diverse_cpp, parse_cpp,
               parse_header, serde_cpp, serde_header, feature_cpp, features_tx,
               compute_tx, baseline_samples)
    before = {str(path.relative_to(root)): sha256(path) for path in sources}
    programs = build()
    results = {
        "diverse": compare(programs["diverse_tx"], programs["diverse_cpp"],
                           diverse_checksums, args.rounds),
        "diverse_clang": compare(programs["diverse_tx"],
                                 programs["diverse_cpp_clang"],
                                 diverse_checksums, args.rounds),
        "heap": compare(programs["features_tx"], programs["feature_cpp"],
                        {"heap_push_pop": "24975000"}, args.rounds),
        "heap_clang": compare(programs["features_tx"],
                              programs["feature_cpp_clang"],
                              {"heap_push_pop": "24975000"}, args.rounds),
        "statistics": compare(programs["compute_tx"], programs["feature_cpp"],
                              {"statistics_mean": "630000"}, args.rounds),
        "statistics_clang": compare(programs["compute_tx"],
                                    programs["feature_cpp_clang"],
                                    {"statistics_mean": "630000"}, args.rounds),
        "cpp_parse_core": parse_core(programs["diverse_cpp"], args.rounds),
        "cpp_parse_core_clang": parse_core(programs["diverse_cpp_clang"], args.rounds),
    }
    after = {str(path.relative_to(root)): sha256(path) for path in sources}
    if before != after:
        raise RuntimeError("测量期间工具链或基准源码变化")
    report = {
        "measured_at_utc": datetime.now(timezone.utc).isoformat(),
        "toolchain_and_source_sha256": after,
        "program_sha256": {name: sha256(path) for name, path in programs.items()},
        "cpp_flags": {"gcc": "g++ -std=c++23 -O3 -DNDEBUG",
                      "clang": "bundled clang -target x86_64-w64-windows-gnu "
                               "-std=c++23 -O3 -DNDEBUG with GCC 13.1 MinGW headers/libs"},
        "results": results,
    }
    output = work / "results_complete.json"
    output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n",
                      encoding="utf-8")
    print(f"已保存 01 等价对照的 {args.rounds} 轮原始样本：{output}")


if __name__ == "__main__":
    main()
