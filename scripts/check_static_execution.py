"""静态执行体系的定向语义与 IR 检查，不运行性能基准。"""

from pathlib import Path
import os
import re
import subprocess


root = Path(__file__).resolve().parents[1]
output = root / "tx_build/static_execution_checks"


def run(arguments):
    environment = os.environ.copy()
    environment["PATH"] = str(root / "tx") + os.pathsep + environment["PATH"]
    result = subprocess.run([str(argument) for argument in arguments], cwd=root,
                            env=environment, capture_output=True, encoding="utf-8",
                            errors="strict", timeout=120)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    return result.stdout


def function(ir, name):
    match = re.search(rf"^define [^\n]* @tx_fn_m0_{name}_0\([^\n]*\)[^\n]*\{{\n(.*?)^\}}",
                      ir, re.MULTILINE | re.DOTALL)
    assert match, name
    return match.group(1)


def main():
    output.mkdir(parents=True, exist_ok=True)
    source = root / "tests/static_execution.tx"
    executable = output / "static_execution.exe"
    llvm = output / "static_execution.ll"
    run([root / "tx/txc.exe", source, "-o", executable])
    assert "STATIC_EXECUTION_OK" in run([executable])
    run([root / "tx/txc.exe", "emit-llvm", source, "-o", llvm])
    ir = llvm.read_text(encoding="utf-8")
    wide = function(ir, "wide_local")
    assert "@tx_fn_m0_native_score_0_native(" in wide
    assert "@txrt_record_struct_new(" not in wide
    assert "define void @tx_fn_m0_echo_record_0_native(" not in ir
    assert "define ptr @tx_fn_m0_echo_record_0_native(" not in ir
    assert "@txrt_value_clone(" not in function(ir, "vector_helper_sum")
    assert "@txrt_value_clone(" in function(ir, "vector_mutating_sum")
    guard = function(ir, "guard_failure")
    assert "@txrt_sync_local_lock_i64(" in guard
    assert "@txrt_sync_local_destroy_i64(" in guard
    assert "@txrt_sync_lock_i64(" not in guard
    assert "@txrt_sync_lock_i64(" in function(ir, "guard_wait_fallback")
    assert "@txrt_sync_local_lock_i64(" not in function(ir, "guard_wait_fallback")
    scalar_guards = function(ir, "check_guard_scalars")
    for suffix in ("f64", "bool"):
        assert f"@txrt_sync_local_lock_{suffix}(" in scalar_guards
        assert f"@txrt_sync_local_destroy_{suffix}(" in scalar_guards
    regex = function(ir, "check_regex")
    assert regex.count("call i32 @txrt_regex_projected(") == 2
    assert "call i32 @txrt_regex_search(" in regex
    assert "call i32 @txrt_format_specialized(" in function(ir, "check_format")
    assert re.search(r"define internal void @\.serde_schema\.\d+\.write", ir)
    assert re.search(r"define internal void @\.serde_schema\.\d+\.read", ir)
    assert "call void @tx_serde_write_str(" in ir
    assert "call void @tx_serde_read_i64(" in ir
    assert "define internal void @tx_record_m0_message.copy(" in ir
    print("PASS static execution: behavior, ownership fallback, specialized IR")
    native = output / "format_binding_cache.exe"
    run(["g++", "-std=c++23", "-O2", "-finput-charset=UTF-8", "-fexec-charset=UTF-8", "-Isrc",
         root / "tests/strings/format_binding_cache.cpp", root / "tx/libtxstdlib.a", "-Ltx/link",
         "-lwinhttp", "-lws2_32", "-ladvapi32", "-lbcrypt", "-lshell32", "-luser32", "-liconv",
         "-o", native])
    assert "FORMAT_BINDING_CACHE_OK" in run([native])
    print("PASS format binding: values, signatures, budgets, eviction, reentry, threads")


if __name__ == "__main__":
    main()
