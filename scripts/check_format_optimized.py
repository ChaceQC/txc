"""07：只运行格式化边界、相关已有用例和 IR/缓存结构检查。"""

from pathlib import Path
import json
import os
import re
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
out = root / "tx_build/format_checks"


def run(args):
    environment = os.environ.copy()
    environment["PATH"] = str(root / "tx") + os.pathsep + environment["PATH"]
    result = subprocess.run([str(arg) for arg in args], cwd=root, env=environment,
                            capture_output=True, encoding="utf-8", errors="strict", timeout=120)
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


def body(ir, name):
    match = re.search(rf"^define [^\n]* @tx_fn_m0_{name}_0\([^\n]*\) \{{\n(.*?)^\}}",
                      ir, re.MULTILINE | re.DOTALL)
    assert match, name
    return match.group(1)


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    out.mkdir(parents=True, exist_ok=True)
    for case in ["strings/format_optimized", "containers/static_runtime"]:
        executable = out / (case.replace("/", "_") + ".exe")
        run([root / "tx/txc.exe", root / "tests" / (case + ".tx"), "-o", executable])
        print(run([executable]).strip())
    llvm = out / "format_optimized.ll"
    run([root / "tx/txc.exe", "emit-llvm", root / "tests/strings/format_optimized.tx", "-o", llvm])
    ir = llvm.read_text(encoding="utf-8")
    for name in ["plain", "literal_bytes", "local_template"]:
        text = body(ir, name)
        for forbidden in ["@txrt_value_box_", "@txrt_array_new(", "@txrt_dict_new(",
                          "@txrt_format_format(", "@txrt_format_literal(", "@txrt_format_append_"]:
            assert forbidden not in text, (name, forbidden)
        assert text.count("call i32 @txrt_format_begin(") == 1
    assert "call i32 @txrt_format_plain_bytes(" in body(ir, "literal_bytes")
    assert "@txrt_str_new(" not in body(ir, "literal_bytes")
    assert "@txrt_format_format(" in body(ir, "changed_template")
    assert "@txrt_format_format(" in body(ir, "dynamic")
    print("PASS IR: 静态专用追加、字节借用、局部常量传播及动态回退")
    native = out / "format_cache.exe"
    run(["g++", "-std=c++23", "-O2", "-finput-charset=UTF-8", "-fexec-charset=UTF-8", "-Isrc",
         root / "tests/strings/format_cache.cpp", root / "tx/libtxstdlib.a", "-Ltx/link",
         "-lwinhttp", "-lws2_32", "-ladvapi32", "-lbcrypt", "-lshell32", "-luser32", "-liconv",
         "-o", native])
    print(run([native]).strip())
    report = {"cases": ["strings/format_optimized", "containers/static_runtime", "strings/format_cache.cpp"],
              "ir": {name: {symbol: body(ir, name).count("call i32 @" + symbol + "(")
                            for symbol in ["txrt_format_begin", "txrt_format_plain_i64",
                                           "txrt_format_plain_bool", "txrt_format_plain_str",
                                           "txrt_format_plain_bytes", "txrt_format_format"]}
                     for name in ["plain", "literal_bytes", "local_template", "changed_template", "dynamic"]}}
    (root / "benchmarks/performance_optimization_07_2026-09-29/structure.json").write_text(
        json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
