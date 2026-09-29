"""静态借用修复：固定旧程序、核对 IR/语义、相关基准交替测量。"""

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[1]
work = root / "tx_build/static_borrowing"
archive = root / "benchmarks/static_borrowing_2026-09-30"
groups = {
    "diverse": ("benchmarks/diverse_performance.tx", ["bench_dictionary_keys", "bench_serde_size"]),
    "language": ("benchmarks/language_features/compare.tx", ["bench_runtime_cast"]),
}


def run(*args):
    result = subprocess.run([str(arg) for arg in args], cwd=root,
                            capture_output=True, encoding="utf-8", errors="strict", timeout=240)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    return result.stdout


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def baseline():
    work.mkdir(parents=True, exist_ok=True)
    archive.mkdir(parents=True, exist_ok=True)
    if (archive / "baseline.json").exists():
        raise RuntimeError("基线已存在，拒绝覆盖旧程序")
    evidence = {"head": run("git", "rev-parse", "HEAD").strip(),
                "compiler": digest(root / "tx/txc.exe"),
                "stdlib": digest(root / "tx/libtxstdlib.a"), "sources": {}}
    for name, (relative, calls) in groups.items():
        original = root / relative
        text = original.read_text(encoding="utf-8")
        if name == "language":
            module = (original.parent / "workload.txh").as_posix()
            text = text.replace('"workload.txh"', f'"{module}"')
        text = text[:text.index("def main(")] + "def main() -> int\n{\n"
        text += "".join(f"    {call}()\n" for call in calls) + "    return 0\n}\n"
        source = work / f"{name}.tx"
        source.write_text(text, encoding="utf-8")
        executable = work / f"{name}_old.exe"
        run(root / "tx/txc.exe", source, "-o", executable)
        run(root / "tx/txc.exe", "emit-llvm", source, "-o", work / f"{name}_old.ll")
        evidence["sources"][name] = {"original": relative, "original_sha256": digest(original),
                                     "source_sha256": digest(source), "exe_sha256": digest(executable)}
    (archive / "baseline.json").write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print("BASELINE_OK", flush=True)


def function(ir, name):
    match = re.search(r"define [^\n]*@tx_fn_[^(]*" + name + r"_0\([^\n]*\).*?^}", ir, re.M | re.S)
    assert match, name
    return match.group()


def optimized_ir(source, output):
    run(root / "tx/clang.exe", "--target=x86_64-w64-windows-gnu", "-O3", "-S", "-emit-llvm",
        "-x", "ir", source, "-o", output)
    return output.read_text(encoding="utf-8")


def check():
    evidence = json.loads((archive / "baseline.json").read_text(encoding="utf-8"))
    observations = {}
    for name, (_, calls) in groups.items():
        source = work / f"{name}.tx"
        assert digest(source) == evidence["sources"][name]["source_sha256"]
        assert digest(work / f"{name}_old.exe") == evidence["sources"][name]["exe_sha256"]
        run(root / "tx/txc.exe", source, "-o", work / f"{name}_new.exe")
        llvm = work / f"{name}.ll"
        optimized = work / f"{name}_optimized.ll"
        run(root / "tx/txc.exe", "emit-llvm", source, "-o", llvm)
        ir = optimized_ir(llvm, optimized)
        old_ir = optimized_ir(work / f"{name}_old.ll", work / f"{name}_old_optimized.ll")
        for call in calls:
            observations[call] = {}
            for label, code in [("old", old_ir), ("new", ir)]:
                body = function(code, call)
                observations[call][label] = {symbol: body.count("@" + symbol + "(") for symbol in
                    ["txrt_str_new", "txrt_value_clone", "txrt_value_to_str", "txrt_record_class_view",
                     "txrt_dictionary_contains_literal", "txrt_record_str_len", "txrt_record_require_type",
                     "txrt_gc_safepoint_context"]}
        if name == "diverse":
            query = function(ir, "bench_dictionary_keys")
            assert "@txrt_dictionary_contains_literal(" in query
            assert "@txrt_dictionary_contains_str(" not in query
            serde = function(ir, "bench_serde_size")
            assert "@txrt_value_clone(" not in serde
            assert "@txrt_value_to_str(" not in serde
            assert "@txrt_record_str_len(" in serde
        else:
            cast = function(ir, "bench_runtime_cast")
            assert "@txrt_value_clone(" not in cast
            assert "@txrt_record_require_type(" in cast
            loop = cast[cast.index("call void @txrt_record_require_type"):
                        cast.index("call i32 @txrt_str_new")]
            for symbol in ["txrt_record_class_view", "txrt_value_release", "txrt_gc_safepoint_context"]:
                assert "@" + symbol + "(" not in loop, symbol
    for case in ["static_borrowing", "static_borrowing_lifetime", "serde/behavior",
                 "serde/direct", "performance_hotspots/virtual_views"]:
        executable = work / (case.replace("/", "_") + ".exe")
        run(root / "tx/txc.exe", root / "tests" / (case + ".tx"), "-o", executable)
        print(run(executable).strip(), flush=True)
    lifetime = work / "lifetime.ll"
    run(root / "tx/txc.exe", "emit-llvm", root / "tests/static_borrowing_lifetime.tx", "-o", lifetime)
    ir = lifetime.read_text(encoding="utf-8")
    assert "@txrt_value_clone(" not in function(ir, "loop_view")
    assert "@txrt_value_clone(" in function(ir, "same_scope")
    assert "@txrt_value_clone(" in function(ir, "early_return")
    # 语言不支持反斜杠零转义；在专用派生源码中放入真实零字节，核对 ABI 的显式长度。
    nul_source = work / "embedded_nul.tx"
    nul_source.write_text('import "dictionary.txh" as dictionary\n'
                          'def main() -> int\n{\n'
                          '    dict values = {"a\0b": 7}\n'
                          '    if !dictionary.contains(values, "a\0b") || dictionary.contains(values, "a")\n'
                          '    {\n        return 1\n    }\n'
                          '    return (dictionary.get(values, "a\0b") as int) - 7\n}\n', encoding="utf-8")
    run(root / "tx/txc.exe", nul_source, "-o", work / "embedded_nul.exe")
    run(work / "embedded_nul.exe")
    print("EMBEDDED_NUL_OK", flush=True)
    print(run("python", "-X", "utf8", "-B", root / "scripts/check_package_compatibility.py").strip())
    (archive / "ir_checks.json").write_text(json.dumps(observations, indent=2) + "\n", encoding="utf-8")
    changed = run("git", "diff", "--name-only", "--", "src").splitlines()
    candidate = {"head": run("git", "rev-parse", "HEAD").strip(),
                 "compiler": digest(root / "tx/txc.exe"), "stdlib": digest(root / "tx/libtxstdlib.a"),
                 "clang": run(root / "tx/clang.exe", "--version").splitlines()[0],
                 "target": "x86_64-w64-windows-gnu", "optimization": "-O3",
                 "changed_source_sha256": {name: digest(root / name) for name in changed},
                 "checks": ["static_borrowing", "static_borrowing_lifetime", "serde/behavior", "serde/direct", "virtual_views",
                            "embedded_nul", "package_compatibility"]}
    (archive / "candidate.json").write_text(json.dumps(candidate, indent=2) + "\n", encoding="utf-8")
    print("IR_AND_SEMANTICS_OK", flush=True)


def measure():
    for name, (relative, _) in groups.items():
        run("python", "-X", "utf8", "-B", root / "scripts/compare_performance_programs.py",
            "--old", work / f"{name}_old.exe", "--new", work / f"{name}_new.exe",
            "--source", work / f"{name}.tx", "--output", archive / f"{name}.json", "--rounds", "5")
        print(f"MEASURE_OK {relative}", flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--baseline", action="store_true")
    mode.add_argument("--check", action="store_true")
    mode.add_argument("--measure", action="store_true")
    args = parser.parse_args()
    if args.baseline:
        baseline()
    elif args.check:
        check()
    else:
        measure()
