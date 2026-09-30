"""只验证稳定向量头、静态格式/serde 和本轮库内优化涉及的行为。"""

from pathlib import Path
import os
import re
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "tx_build" / "static_runtime_checks"
COMPILER = ROOT / "tx" / "txc.exe"


def run(arguments):
    environment = os.environ.copy()
    environment["PATH"] = str(ROOT / "tx") + os.pathsep + environment["PATH"]
    result = subprocess.run([str(value) for value in arguments], cwd=ROOT,
                            env=environment, capture_output=True, encoding="utf-8",
                            errors="strict", timeout=60)
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


def body(ir, name):
    match = re.search(rf"^define [^\n]* @tx_fn_m0_{name}_0\([^\n]*\)[^\n]*\{{\n(.*?)^\}}",
                      ir, re.MULTILINE | re.DOTALL)
    assert match, name
    return match.group(1)


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    OUT.mkdir(parents=True, exist_ok=True)
    cases = sys.argv[1:] or [
        "containers/static_runtime", "containers/call_borrowing",
        "serde/behavior", "serde/module", "formats/serde_migration",
        "statistics/basic", "bytes_file_stream/behavior",
        "bytes_file_stream/encodings", "strings/regex_behavior",
    ]
    for case in cases:
        executable = OUT / (case.replace("/", "_") + ".exe")
        run([COMPILER, ROOT / "tests" / (case + ".tx"), "-o", executable])
        print(f"PASS {case}: {run([executable]).strip()}")
    llvm = OUT / "static_runtime.ll"
    run([COMPILER, "emit-llvm", ROOT / "tests/containers/static_runtime.tx", "-o", llvm])
    ir = llvm.read_text(encoding="utf-8")
    for name in ("vector_sum", "vector_reads", "changing_vector"):
        assert body(ir, name).count("call ptr @txrt_vector_ref_i64(") == 1, name
    assert "call void @txrt_vector_index_error(" not in body(ir, "vector_sum")
    assert "call void @txrt_vector_index_error(" in body(ir, "changing_vector")
    direct = body(ir, "direct_format")
    assert "call i32 @txrt_format_append_i64(" in direct
    assert "call i32 @txrt_format_plain_str(" in direct
    for forbidden in ("@txrt_value_box_", "@txrt_array_new(", "@txrt_dict_new(",
                      "@txrt_format_format("):
        assert forbidden not in direct, forbidden
    assert "@.serde_schema." in ir and 'private constant { ptr, ptr, i64, i64' in ir
    assert "serde_parse_schema" not in (ROOT / "src/stdlib/serde_schema.cpp").read_text(encoding="utf-8")
    encoding = body(ir, "constant_encoding")
    assert "@txrt_encoding_encode_known(" in encoding
    assert "@txrt_str_new(" not in encoding
    print("PASS IR: 向量恢复移出热路径，稳定遍历免检，格式化无动态实参容器，serde 静态描述")
    for name in ("runtime_workspace", "regex_cancel"):
        executable = OUT / (name + ".exe")
        run(["g++", "-std=c++23", "-O2", "-finput-charset=UTF-8", "-fexec-charset=UTF-8",
             "-Isrc", ROOT / "tests/strings" / (name + ".cpp"), ROOT / "tx/libtxstdlib.a",
             "-Ltx/link", "-lwinhttp", "-lws2_32", "-ladvapi32", "-lbcrypt", "-lshell32",
             "-luser32", "-liconv", "-o", executable])
        print(f"PASS {name}: {run([executable]).strip()}")


if __name__ == "__main__":
    main()
