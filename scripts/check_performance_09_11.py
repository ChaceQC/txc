"""09–11 定向结构与行为检查，不执行无关模块全量套件。"""
from pathlib import Path
import argparse
import json
import os
import re
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / "tx_build/performance_09_11/checks"
archive = root / "benchmarks/performance_optimization_09_11_2026-09-29"
compiler = root / "tx/txc.exe"
environment = os.environ.copy()
environment["PATH"] = str(root / "tx") + os.pathsep + environment["PATH"]


def run(*arguments, expected=0):
    result = subprocess.run([str(value) for value in arguments], cwd=root,
                            env=environment, capture_output=True, encoding="utf-8",
                            errors="strict", timeout=120)
    assert result.returncode == expected, (arguments, result.returncode, result.stdout, result.stderr)
    if expected:
        assert "guard deinit" in result.stdout and "运行错误" in result.stderr
    return (result.stdout + (result.stderr if expected else "")).strip()


def function(ir, name):
    match = re.search(rf"^define [^\n]* @tx_fn_m0_{name}_0\([^\n]*\)[^\n]*\{{\n(.*?)^\}}",
                      ir, re.MULTILINE | re.DOTALL)
    assert match, name
    return match.group(1)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("cases", nargs="*")
    args = parser.parse_args()
    out.mkdir(parents=True, exist_ok=True)
    cases = args.cases or [
        "tests/performance_09_11/behavior", "tests/performance_09_11/dynamic_fields",
        "tests/performance_09_11/module_identity",
        "examples/closures", "examples/function_values",
        "tests/stdlib/closure_module/main", "tests/stdlib/closure_error",
        "tests/stdlib/closure_cycle", "tests/stdlib/thread_behavior",
        "tests/stdlib/task_graph_lifecycle", "examples/advanced_classes",
        "examples/advanced_class_module/main", "examples/cycle_resurrection",
        "examples/cycle_collection", "examples/memory_error_cleanup",
        "tests/containers/sequence_extended_contract", "tests/containers/ordered_behavior",
        "tests/containers/behavior", "tests/serde/direct", "tests/serde/behavior",
        "tests/containers/call_borrowing",
        "tests/serde/module", "tests/formats/serde_migration",
    ]
    records = {}
    for case in cases:
        executable = out / (case.replace("/", "_") + ".exe")
        run(compiler, root / (case + ".tx"), "-o", executable)
        records[case] = run(executable, expected=1 if case == "examples/memory_error_cleanup" else 0)
        if case == "examples/cycle_collection":
            for message in ("link deinit 1", "link deinit 2", "link deinit 3", "link deinit 4",
                            "array owner deinit", "dict owner deinit"):
                assert records[case].splitlines().count(message) == 2, (case, message)
        if case == "examples/cycle_resurrection":
            assert records[case].splitlines() == ["victim deinit", "7"]
        print(f"PASS {case}: {records[case]}", flush=True)
    if args.cases:
        return
    llvm = out / "paths.ll"
    run(compiler, "emit-llvm", archive / "paths.tx", "-o", llvm)
    ir = llvm.read_text(encoding="utf-8")
    assert "call ptr @txrt_struct_field_" not in ir
    assert "call ptr @txrt_class_field_" not in ir
    assert "call i32 @txrt_struct_set_field" not in ir
    calls = function(ir, "bench_calls")
    assert "call ptr @txrt_closure_code" not in calls
    assert "@tx_callback_m0_double_value_0_internal(ptr %tx_context" in calls
    internal = re.findall(r"^define [^\n]*_internal\([^\n]*\)[^\n]*\{\n(.*?)^\}",
                          ir, re.MULTILINE | re.DOTALL)
    assert internal and all("call ptr @txrt_runtime_context" not in body for body in internal)
    assert "@txrt_closure_capture" not in "\n".join(internal)
    heap = function(ir, "bench_heap")
    assert "@txrt_value_clone" not in heap
    native = out / "heap_native.exe"
    run("g++", "-std=c++23", "-O2", "-finput-charset=UTF-8", "-fexec-charset=UTF-8",
        "-Isrc", root / "tests/performance_09_11/heap_native.cpp", root / "tx/libtxstdlib.a",
        "-Ltx/link", "-lwinhttp", "-lws2_32", "-ladvapi32", "-lbcrypt", "-lshell32",
        "-luser32", "-liconv", "-o", native)
    records["heap_native"] = run(native)
    print("PASS static fields, internal context, scalar captures, borrowed heap, native heap")
    (archive / "checks.json").write_text(json.dumps(records, ensure_ascii=False, indent=2)
                                         + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
