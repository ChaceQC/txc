"""冻结热点优化的实际工作树与发布包；所有文本产物使用 UTF-8。"""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import statistics
import subprocess

from compare_performance_programs import check, read_program


root = Path(__file__).resolve().parents[1]
work = root / "tx_build/performance_hotspots"
archive = root / "benchmarks/performance_hotspots_2026-09-29"
sources = {
    "stage": Path("E:/Project/problems/not-yet-on-stage/src/solution_benchmark.tx"),
    "diverse": root / "benchmarks/diverse_performance.tx",
    "language": root / "benchmarks/language_features/compare.tx",
    "parse": root / "benchmarks/performance_optimization_06_2026-09-29/parse_paths.tx",
    "format": root / "benchmarks/performance_optimization_07_2026-09-29/format_paths.tx",
    "serde": root / "benchmarks/performance_optimization_08_2026-09-29/serde_paths.tx",
    "encoding": root / "benchmarks/performance_optimization_15_16_2026-09-29/paths.tx",
}


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(*args: object) -> str:
    result = subprocess.run([str(arg) for arg in args], cwd=root, capture_output=True,
                            encoding="utf-8", errors="strict", timeout=300)
    if result.returncode:
        raise RuntimeError(f"{args}: {result.stdout}\n{result.stderr}")
    return result.stdout


def save(path: Path, value: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def freeze() -> None:
    destination = work / "before"
    if destination.exists():
        raise RuntimeError("基线目录已存在；禁止覆盖原始发布包")
    destination.mkdir(parents=True)
    shutil.copytree(root / "tx", destination / "tx")
    paths = [*root.glob("src/**/*.cpp"), *root.glob("src/**/*.hpp"),
             *root.glob("tx/stdlib/**/*.txh"), root / "CMakeLists.txt", root / "scripts/build.ps1"]
    manifest = {
        "created_at_utc": datetime.now(timezone.utc).isoformat(),
        "head": run("git", "rev-parse", "HEAD").strip(),
        "status": run("git", "status", "--short"),
        "source_hashes": {str(path.relative_to(root)): sha256(path) for path in sorted(paths)},
        "package_hashes": {str(path.relative_to(destination / "tx")): sha256(path)
                           for path in sorted((destination / "tx").rglob("*")) if path.is_file()},
        "benchmark_hashes": {name: {"path": str(path), "sha256": sha256(path)}
                             for name, path in sources.items()},
    }
    (destination / "tracked.patch").write_text(run("git", "diff", "--no-ext-diff"), encoding="utf-8")
    for path in paths:
        target = destination / "source" / path.relative_to(root)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(path, target)
    save(archive / "before.json", manifest)
    compile_programs(destination / "tx", destination)
    print("PASS baseline frozen", flush=True)


def compile_programs(package: Path, destination: Path) -> None:
    destination.mkdir(parents=True, exist_ok=True)
    for path in package.glob("*.dll"):
        shutil.copyfile(path, destination / path.name)
    for name, source in sources.items():
        run(package / "txc.exe", source, "-o", destination / f"{name}.exe")
        run(package / "txc.exe", "emit-llvm", source, "-o", destination / f"{name}.ll")
        print(f"PASS compiled {destination.name}/{name}", flush=True)


def checks(selected: list[str] | None) -> None:
    destination = work / "checks"
    destination.mkdir(parents=True, exist_ok=True)
    cases = {
        "tests/performance_hotspots/local_array": "LOCAL_ARRAY_OK",
        "tests/performance_hotspots/virtual_views": "VIRTUAL_VIEWS_OK",
        "tests/performance_hotspots/overflow": "整数加法溢出\n整数减法溢出\n整数乘法溢出\nOVERFLOW_OK",
        "tests/stdlib/parse_scalar": "PARSE_SCALAR_OK",
        "tests/performance_hotspots/parse_error": "PARSE_ERROR_OK",
        "tests/performance_hotspots/format_dynamic": "FORMAT_DYNAMIC_OK",
        "tests/performance_hotspots/dictionary_iteration": "DICTIONARY_ITERATION_OK",
        "tests/strings/format_optimized": "FORMAT_OPTIMIZED_OK",
        "tests/serde/direct": None,
        "tests/performance_hotspots/serde_wide": "SERDE_WIDE_OK",
        "tests/serde/behavior": None,
        "tests/serde/module": None,
        "tests/performance_09_11/behavior": None,
        "examples/advanced_classes": None,
        "examples/advanced_class_module/main": None,
        "examples/cycle_resurrection": "victim deinit\n7",
    }
    result_path = archive / "checks.json"
    results = json.loads(result_path.read_text(encoding="utf-8")) if result_path.exists() else {}
    for source, expected in cases.items():
        if selected and source not in selected:
            continue
        program = destination / (source.replace("/", "_") + ".exe")
        run(root / "tx/txc.exe", root / (source + ".tx"), "-o", program)
        output = run(program).strip()
        if expected is not None and output != expected:
            raise AssertionError((source, output, expected))
        results[source] = {"output": output, "source_sha256": sha256(root / (source + ".tx")),
                           "program_sha256": sha256(program)}
        print(f"PASS {source}", flush=True)
    save(archive / "checks.json", results)


def measure(selected: list[str], report: str) -> None:
    results = {}
    snapshot_paths = [*root.glob("src/**/*.cpp"), *root.glob("src/**/*.hpp"),
                      *root.glob("tx/stdlib/*.txh"), *root.glob("tx/*.dll"),
                      root / "tx/txc.exe", root / "tx/libtxstdlib.a", root / "tx/package.compat"]
    snapshot = {str(path.relative_to(root)): sha256(path) for path in sorted(snapshot_paths)}
    for name in sources:
        if name == "stage" or name not in selected:
            continue
        programs = {label: work / label / f"{name}.exe" for label in ("before", "after")}
        expected = read_program(programs["before"])
        check(expected, read_program(programs["after"]))
        samples = {label: [] for label in programs}
        for index in range(5):
            for label in (("before", "after") if index % 2 == 0 else ("after", "before")):
                values = read_program(programs[label])
                check(expected, values)
                samples[label].append(values)
        results[name] = {
            "sha256": {label: sha256(path) for label, path in programs.items()},
            "warmups": 1, "rounds": 5, "samples": samples,
            "medians_ms": {label: {key: statistics.median(row[key][0] for row in rows)
                                   for key in expected} for label, rows in samples.items()},
        }
        save(archive / report, results)
        print(f"PASS paired {name}", flush=True)
    data = Path("E:/Project/problems/not-yet-on-stage/testdata")
    stage_cases = ("all-free-max", "forced-increasing-max", "forced-decreasing-max",
                   "shuffled-tight-max-1", "alternating-tight-max") if "stage" in selected else ()
    for name in stage_cases:
        input_data = (data / f"{name}.in").read_bytes()
        expected = (data / f"{name}.out").read_bytes().split()
        samples = {label: [] for label in ("before", "after")}
        for index in range(6):
            for label in (("before", "after") if index % 2 == 0 else ("after", "before")):
                result = subprocess.run([str(work / label / "stage.exe")], input=input_data,
                                        capture_output=True, timeout=30, check=True)
                if result.stdout.split() != expected:
                    raise AssertionError((name, label, "外部题答案不一致"))
                if index:
                    samples[label].append(float(result.stderr.decode("utf-8").strip()) / 1000)
        results["stage/" + name] = {
            "input_sha256": sha256(data / f"{name}.in"), "warmups": 1, "rounds": 5,
            "program_sha256": {label: sha256(work / label / "stage.exe") for label in samples},
            "samples_ms": samples,
            "medians_ms": {label: statistics.median(rows) for label, rows in samples.items()},
        }
        save(archive / report, results)
        print(f"PASS paired stage/{name}", flush=True)
    if snapshot != {str(path.relative_to(root)): sha256(path) for path in sorted(snapshot_paths)}:
        raise RuntimeError("采样期间源码或发布包发生变化")
    save(archive / "after.json", {"measured_at_utc": datetime.now(timezone.utc).isoformat(),
                                  "hashes": snapshot})


def inspect() -> None:
    def function(ir: str, suffix: str) -> str:
        found = re.search(r"^define [^\n]*@tx_fn_[^(]*" + re.escape(suffix) +
                          r"\([^\n]*\)[^{]*\{\n(.*?)^\}", ir, re.MULTILINE | re.DOTALL)
        if not found:
            raise AssertionError(suffix)
        return found.group(1)

    records = {}
    for name in ("parse", "format", "language", "diverse", "stage"):
        source = work / "after" / f"{name}.ll"
        output = work / "after" / f"{name}_optimized.ll"
        run(root / "tx/clang.exe", "-target", "x86_64-w64-windows-gnu", "-O3",
            "-S", "-emit-llvm", source, "-o", output)
        records[name] = output.read_text(encoding="utf-8")
    parsed = function(records["parse"], "full_result_0")
    assert "@txrt_parse_error_field_length" in parsed
    assert not any(token in parsed for token in ("@txrt_parse_materialize", "@txrt_gc_safepoint",
                                                 "@txrt_struct_field", "@txrt_value_clone"))
    dynamic = function(records["format"], "dynamic_format_0")
    assert "@txrt_format_arguments_context" in dynamic
    assert "@txrt_array_new" not in dynamic and "@txrt_dict_new" not in dynamic
    dictionary = function(records["language"], "bench_dict_iteration_0")
    assert "@txrt_dictionary_keys" not in dictionary
    virtual = function(records["language"], "bench_virtual_interface_0")
    # 初始化和报告仍有安全点；循环机器码单独归档，不能用全函数计数替代热路径判断。
    assert "getelementptr" in virtual
    source = root / "tests/performance_hotspots/virtual_views.tx"
    for mode in ("emit-llvm", "emit-library-llvm"):
        output = work / "checks" / (mode + ".ll")
        run(root / "tx/txc.exe", mode, source, "-o", output)
        body = function(output.read_text(encoding="utf-8"), "sum_reads_0")
        has_safepoint = "@txrt_gc_safepoint_context" in body
        assert has_safepoint == (mode == "emit-library-llvm"), (mode, body)
    for suite, symbol in (("diverse", "tx_fn_m0_bench_vector_scale_0"),
                          ("language", "tx_fn_m0_bench_virtual_interface_0"),
                          ("stage", "tx_fn_main_0")):
        for label in ("before", "after"):
            output = run("objdump", "-d", "--no-show-raw-insn", "--disassemble=" + symbol,
                         work / label / f"{suite}.exe")
            (archive / f"{label}_{suite}.asm").write_text(output, encoding="utf-8")
    compatibility = run("python", "-X", "utf8", "-B", root / "scripts/check_package_compatibility.py")
    members = run("ar", "t", root / "tx/libtxstdlib.a").splitlines()
    for name in ("httpx_bridge.o", "websocket_bridge.o", "requests_bridge.o"):
        assert members.count(name) == 1, name
    save(archive / "structure.json", {
        "readonly_parse": "no result/error materialization or per-iteration safepoint",
        "dynamic_format": "stack arguments without array/dict",
        "dictionary": "unused keys with stable scalar body avoid snapshot",
        "virtual_library_boundary": "regular program proven pure; library remains conservative",
        "compatibility": compatibility.strip(), "bridge_count_each": 1,
    })
    print("PASS generated code, library virtual boundary and package compatibility", flush=True)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("freeze", "compile", "checks", "measure", "inspect"))
    parser.add_argument("--suites", nargs="+", choices=tuple(sources), default=list(sources))
    parser.add_argument("--report", default="paired.json")
    parser.add_argument("--cases", nargs="+")
    args = parser.parse_args()
    os.environ["PYTHONUTF8"] = "1"
    os.environ["PATH"] = str(root / "tx") + os.pathsep + os.environ["PATH"]
    if args.mode == "freeze":
        freeze()
    elif args.mode == "compile":
        compile_programs(root / "tx", work / "after")
    elif args.mode == "checks":
        checks(args.cases)
    elif args.mode == "inspect":
        inspect()
    else:
        measure(args.suites, args.report)


if __name__ == "__main__":
    main()
