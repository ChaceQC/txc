"""复测多维度 TX 负载，并保留计时、校验值和进程内存样本。"""

from __future__ import annotations

from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import statistics
import subprocess
import sys
import time

import psutil


root = Path(__file__).resolve().parent.parent
compiler = root / "tx/txc.exe"
library = root / "tx/libtxstdlib.a"
source = root / "benchmarks/diverse_performance.tx"
cpp_source = root / "benchmarks/diverse_performance.cpp"
cpp_parse_source = root / "benchmarks/performance_equivalence/parse_contract.cpp"
cpp_parse_header = root / "benchmarks/performance_equivalence/parse_contract.hpp"
cpp_serde_source = root / "benchmarks/performance_equivalence/serde_payload.cpp"
cpp_serde_header = root / "benchmarks/performance_equivalence/serde_payload.hpp"
python_source = root / "benchmarks/diverse_performance.py"
java_source = root / "benchmarks/diverse_performance_java.java"
startup_source = root / "benchmarks/minimal_startup.tx"
cpp_startup_source = root / "benchmarks/minimal_startup.cpp"
python_startup_source = root / "benchmarks/minimal_startup.py"
java_startup_source = root / "benchmarks/minimal_startup_java.java"
program = root / "tx_build/diverse_performance.exe"
cpp_program = root / "tx_build/diverse_performance_cpp.exe"
startup_program = root / "tx_build/minimal_startup.exe"
cpp_startup_program = root / "tx_build/minimal_startup_cpp.exe"
output = root / "tx_build/diverse_performance_results_equivalent.json"
jackson_root = Path.home() / ".m2/repository/com/fasterxml/jackson/core"
jackson_jars = [
    jackson_root / "jackson-databind/2.18.4/jackson-databind-2.18.4.jar",
    jackson_root / "jackson-core/2.18.4.1/jackson-core-2.18.4.1.jar",
    jackson_root / "jackson-annotations/2.18.4/jackson-annotations-2.18.4.jar",
]
java_classpath = os.pathsep.join([str(root / "tx_build"), *map(str, jackson_jars)])

expected_checksums = {
    "vector_scan_1k": 3000000,
    "vector_scan_100k": 30000000,
    "vector_index_sequential": 47857475,
    "vector_index_strided": 47857965,
    "map_hit_128": 31748464,
    "map_hit_8192": 2047690384,
    "map_hit_10_percent": 50000,
    "dictionary_int_hit": 200000,
    "dictionary_text_hit": 200000,
    "dictionary_mostly_miss": 1,
    "format_literal": 488890,
    "format_dynamic": 488890,
    "encoding_literal": 900000,
    "encoding_dynamic": 900000,
    "serde_short_text": 40000,
    "serde_long_text": 655000,
    "parse_valid": 100000,
    "parse_invalid": 100000,
}


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run_measured(command: list[str]) -> dict:
    started = time.perf_counter()
    process = psutil.Popen(
        command, cwd=root, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    sampled_tree_peak = 0
    sampled_process_peak = 0
    while True:
        # 编译器会启动 clang；进程树工作集比只看 txc 更接近实际峰值。
        try:
            members = [process, *process.children(recursive=True)]
            tree_working_set = sum(member.memory_info().rss for member in members)
            sampled_tree_peak = max(sampled_tree_peak, tree_working_set)
            sampled_process_peak = max(
                sampled_process_peak, process.memory_info().peak_wset
            )
        except psutil.Error:
            pass
        if process.poll() is not None:
            break
        time.sleep(0.002)
    stdout, stderr = process.communicate()
    wall_ms = (time.perf_counter() - started) * 1000
    if process.returncode != 0:
        raise RuntimeError(
            f"{command}: exit {process.returncode}: "
            f"{stderr.decode('utf-8', errors='replace')[-1000:]}"
        )
    return {
        "wall_ms": wall_ms,
        "sampled_tree_peak_mib": sampled_tree_peak / 1048576,
        "sampled_process_peak_mib": sampled_process_peak / 1048576,
        "stdout": stdout.decode("utf-8", errors="strict"),
    }


def parse_results(text: str) -> dict[str, dict]:
    lines = text.splitlines()
    if len(lines) != len(expected_checksums) * 3:
        raise ValueError(f"Expected {len(expected_checksums) * 3} lines, got {len(lines)}")
    results = {}
    for index in range(0, len(lines), 3):
        name = lines[index]
        if name in results or name not in expected_checksums:
            raise ValueError(f"Unexpected or duplicate benchmark: {name}")
        elapsed_us = int(lines[index + 1])
        checksum = int(lines[index + 2])
        if checksum != expected_checksums[name]:
            raise ValueError(f"Wrong checksum for {name}: {checksum}")
        results[name] = {"elapsed_us": elapsed_us, "checksum": checksum}
    return results


def summarize(samples: list[dict], key: str) -> dict:
    values = [sample[key] for sample in samples]
    captured = [value for value in values if value > 0]
    return {"median": statistics.median(captured) if captured else None,
            "samples": values}


def run_cross_language(commands: dict[str, list[str]], rounds: int = 5) -> dict:
    for command in commands.values():
        parse_results(run_measured(command)["stdout"])
    samples = {language: [] for language in commands}
    labels = list(commands)
    for index in range(rounds):
        # 每轮轮换起始语言，避免固定顺序总让同一语言承受冷态。
        order = labels[index % len(labels):] + labels[:index % len(labels)]
        for language in order:
            measurement = run_measured(commands[language])
            measurement["cases"] = parse_results(measurement["stdout"])
            samples[language].append(measurement)
        print(f"Runtime round {index + 1}/{rounds}: {', '.join(order)}", flush=True)
    cases = {}
    for name in expected_checksums:
        cases[name] = {language: {
            "median_ms": statistics.median(
                sample["cases"][name]["elapsed_us"] for sample in group
            ) / 1000,
            "samples_us": [sample["cases"][name]["elapsed_us"] for sample in group],
            "checksum": expected_checksums[name],
        } for language, group in samples.items()}
    processes = {language: {
        "wall_ms": summarize(group, "wall_ms"),
        "sampled_tree_peak_mib": summarize(group, "sampled_tree_peak_mib"),
    } for language, group in samples.items()}
    return {"warmups": 1, "rounds": rounds, "cases": cases, "processes": processes}


def run_startup(commands: dict[str, list[str]], rounds: int = 7) -> dict:
    for command in commands.values():
        if run_measured(command)["stdout"]:
            raise ValueError("Minimal startup program unexpectedly wrote output")
    samples = {language: [] for language in commands}
    labels = list(commands)
    for index in range(rounds):
        order = labels[index % len(labels):] + labels[:index % len(labels)]
        for language in order:
            sample = run_measured(commands[language])
            if sample["stdout"]:
                raise ValueError("Minimal startup program unexpectedly wrote output")
            samples[language].append(sample)
    return {language: {
        "wall_ms": summarize(group, "wall_ms"),
        "sampled_tree_peak_mib": summarize(group, "sampled_tree_peak_mib"),
    } for language, group in samples.items()}


def run_compilation(command: list[str], rounds: int = 3) -> dict:
    run_measured(command)
    samples = [run_measured(command) for _ in range(rounds)]
    return {
        "warmups": 1,
        "rounds": rounds,
        "wall_ms": summarize(samples, "wall_ms"),
        "sampled_tree_peak_mib": summarize(samples, "sampled_tree_peak_mib"),
    }


def run_cpp_parse_core(rounds: int = 5) -> dict:
    command = [str(cpp_program), "--parse-core"]
    expected = {"parse_core_valid": 100000, "parse_core_invalid": 100000}

    def read() -> dict[str, int]:
        lines = run_measured(command)["stdout"].splitlines()
        if len(lines) != 6:
            raise ValueError("C++ 核心解析对照输出行数错误")
        result = {lines[index]: (int(lines[index + 1]), int(lines[index + 2]))
                  for index in (0, 3)}
        if set(result) != set(expected) or any(
            result[name][1] != checksum for name, checksum in expected.items()
        ):
            raise ValueError("C++ 核心解析对照校验失败")
        return {name: result[name][0] for name in expected}

    read()
    samples = [read() for _ in range(rounds)]
    return {name: {"checksum": expected[name],
                   "samples_us": [item[name] for item in samples],
                   "median_ms": statistics.median(item[name] for item in samples) / 1000}
            for name in expected}


def main() -> None:
    before = {str(path.relative_to(root)): digest(path) for path in
              (compiler, library, source, cpp_source, cpp_parse_source,
               cpp_parse_header, cpp_serde_source, cpp_serde_header,
               python_source, java_source,
               startup_source, cpp_startup_source, python_startup_source,
               java_startup_source)}
    if any(not jar.is_file() for jar in jackson_jars):
        raise FileNotFoundError("Jackson 2.18.4 jars required for Java JSON comparison")
    run_measured([str(compiler), str(source), "-o", str(program)])
    run_measured([str(compiler), str(startup_source), "-o", str(startup_program)])
    cpp_flags = ["g++", "-std=c++23", "-O3", "-DNDEBUG"]
    cpp_command = [*cpp_flags, str(cpp_source), str(cpp_parse_source),
                   str(cpp_serde_source), "-o", str(cpp_program)]
    run_measured(cpp_command)
    run_measured([str(cpp_program), "--contract-check"])
    run_measured([*cpp_flags, str(cpp_startup_source), "-o",
                  str(cpp_startup_program)])
    run_measured(["javac", "-encoding", "UTF-8", "-cp", java_classpath,
                  "-d", str(root / "tx_build"), str(java_source),
                  str(java_startup_source)])

    commands = {
        "TX": [str(program)],
        "C++": [str(cpp_program)],
        "Python": [sys.executable, "-X", "utf8", "-B", str(python_source)],
        "Java": ["java", "-cp", java_classpath, "diverse_performance_java"],
    }
    startup_commands = {
        "TX": [str(startup_program)],
        "C++": [str(cpp_startup_program)],
        "Python": [sys.executable, "-X", "utf8", "-B", str(python_startup_source)],
        "Java": ["java", "-cp", java_classpath, "minimal_startup_java"],
    }
    diverse = run_cross_language(commands)
    cpp_parse_core = run_cpp_parse_core()
    startup = run_startup(startup_commands)
    compile_results = {
        "TX check": run_compilation([str(compiler), "check", str(source)]),
        "TX full": run_compilation([str(compiler), str(source), "-o", str(program)]),
        "C++ full": run_compilation(cpp_command),
        "Python bytecode": run_compilation([
            sys.executable, "-X", "utf8", "-B", "-c",
            "from pathlib import Path; import sys; "
            "compile(Path(sys.argv[1]).read_text(encoding='utf-8'), sys.argv[1], 'exec')",
            str(python_source),
        ]),
        "Java full": run_compilation([
            "javac", "-encoding", "UTF-8", "-cp", java_classpath,
            "-d", str(root / "tx_build"), str(java_source),
        ]),
    }

    after = {str(path.relative_to(root)): digest(path) for path in
             (compiler, library, source, cpp_source, cpp_parse_source,
              cpp_parse_header, cpp_serde_source, cpp_serde_header,
              python_source, java_source,
              startup_source, cpp_startup_source, python_startup_source,
              java_startup_source)}
    if before != after:
        raise RuntimeError("Compiler, library or benchmark sources changed during measurement")
    report = {
        "measured_at_utc": datetime.now(timezone.utc).isoformat(),
        "toolchain_and_source_sha256": after,
        "platform": "Windows x64",
        "peak_memory_note": "2 ms polling of process-tree working set; approximate",
        "diverse": diverse,
        "cpp_parse_core": cpp_parse_core,
        "comparison_note": "parse_valid/invalid and serde_* use full C++ payload contracts; "
                           "2026-09-29 archived C++ samples used simplified references",
        "startup": startup,
        "compilation": compile_results,
        "program_bytes": {
            "TX": program.stat().st_size,
            "C++": cpp_program.stat().st_size,
            "Python source": python_source.stat().st_size,
            "Java class": (root / "tx_build/diverse_performance_java.class").stat().st_size,
        },
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n",
                      encoding="utf-8")
    for name, values in diverse["cases"].items():
        text = ", ".join(f"{language} {value['median_ms']:.3f} ms"
                         for language, value in values.items())
        print(f"{name}: {text}")
    print(f"Saved results to {output}")


if __name__ == "__main__":
    main()
