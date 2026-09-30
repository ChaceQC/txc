"""12–14 的定向行为、IR 和兼容性验收；不运行全量套件。"""
from pathlib import Path
import argparse
import json
import os
import re
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / "tx_build/performance_12_14/checks"
archive = root / "benchmarks/performance_optimization_12_14_2026-09-29"
environment = os.environ.copy()
environment["PATH"] = str(root / "tx") + os.pathsep + environment["PATH"]


def run(*arguments, expected=0):
    result = subprocess.run([str(value) for value in arguments], cwd=root,
                            env=environment, capture_output=True, encoding="utf-8",
                            errors="strict", timeout=120)
    assert result.returncode == expected, (arguments, result.returncode, result.stdout, result.stderr)
    return (result.stdout + result.stderr).strip()


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
        "tests/performance_12_14/iterators", "tests/performance_12_14/arithmetic",
        "tests/performance_12_14/spreads", "tests/performance_12_14/graphs",
        "examples/iterators", "tests/stdlib/iterator_module/main",
        "tests/statistics/basic", "examples/variadic_unpack", "tests/stdlib/stack_trace",
        "examples/cycle_collection", "examples/cycle_resurrection",
        "examples/memory_error_cleanup", "tests/stdlib/task_graph_lifecycle",
        "tests/stdlib/closure_cycle", "tests/performance_09_11/behavior",
    ]
    records = {}
    for case in cases:
        executable = out / (case.replace("/", "_") + ".exe")
        run(root / "tx/txc.exe", root / (case + ".tx"), "-o", executable)
        records[case] = run(executable, expected=1 if case == "examples/memory_error_cleanup" else 0)
        if case.startswith("tests/performance_12_14/"):
            assert records[case].endswith(case.rsplit("/", 1)[1] + " ok"), records[case]
        if case == "examples/cycle_collection":
            for message in ("link deinit 1", "link deinit 2", "link deinit 3", "link deinit 4",
                            "array owner deinit", "dict owner deinit"):
                assert records[case].splitlines().count(message) == 2, (case, message)
        if case == "examples/cycle_resurrection":
            assert records[case].splitlines() == ["victim deinit", "7"]
        if case == "examples/memory_error_cleanup":
            assert "guard deinit" in records[case] and "运行错误" in records[case]
        if case == "tests/stdlib/stack_trace":
            for name in ("inner", "middle", "main"):
                assert name in records[case]
            assert "stack_trace.tx:5:" in records[case], records[case]
        print("PASS " + case, flush=True)
    if args.cases:
        saved = archive / "checks.json"
        if saved.exists():
            previous = json.loads(saved.read_text(encoding="utf-8"))
            previous.update(records)
            saved.write_text(json.dumps(previous, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        return
    for name in ("iterators", "arithmetic", "spreads"):
        llvm = out / (name + ".ll")
        run(root / "tx/txc.exe", "emit-llvm", root / f"tests/performance_12_14/{name}.tx",
            "-o", llvm)
        ir = llvm.read_text(encoding="utf-8")
        if name == "iterators":
            stable = function(ir, "stable_snapshot")
            assert "@txrt_iterator_next_scalar" not in stable
            assert "@txrt_iterator_snapshot_cursor" in stable and "%tx_iterator_cursor" in stable
            assert "@txrt_iterator_next_scalar" in function(ir, "aliases_and_live")
        if name == "arithmetic":
            assert "sdiv i64" in function(ir, "divide")
            assert "fdiv double" in function(ir, "floating")
            assert "@llvm.ssub.with.overflow" not in function(ir, "bounded")
            assert "@llvm.ssub.with.overflow" in function(ir, "mutable_range")
            assert "@llvm.smul.with.overflow" not in function(ir, "proven")
            assert "@txrt_div_i64" not in function(ir, "proven")
        if name == "spreads":
            literal = function(ir, "literal_spread")
            assert "@txrt_call_bind" not in literal and "@txrt_array_new" not in literal
            dynamic = function(ir, "dynamic_spread")
            assert "@txrt_call_bound_slot" in dynamic
            assert "@txrt_array_element_address" not in dynamic
    records["structure"] = "snapshot view, alias fallback, native division, conservative ranges"
    (archive / "checks.json").write_text(json.dumps(records, ensure_ascii=False, indent=2)
                                         + "\n", encoding="utf-8")
    print("PASS IR checks")


if __name__ == "__main__":
    main()
