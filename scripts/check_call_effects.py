"""定向检查安全查询的 GC 轮询与对象析构边界。"""

from pathlib import Path
import re
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "tests/containers/call_effects.tx"
OUTPUT = ROOT / "tx_build/call_effects_checks"


def run(*arguments):
    result = subprocess.run(
        [str(value) for value in arguments], cwd=ROOT,
        capture_output=True, encoding="utf-8", errors="strict", timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


def function_body(ir, name):
    match = re.search(
        rf"^define [^\n]* @tx_fn_m0_{re.escape(name)}_0\([^\n]*\) \{{\n(.*?)^\}}",
        ir, re.MULTILINE | re.DOTALL,
    )
    assert match, f"缺少函数 {name}"
    return match.group(1)


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    OUTPUT.mkdir(parents=True, exist_ok=True)
    compiler = ROOT / "tx/txc.exe"
    executable = OUTPUT / "behavior.exe"
    llvm = OUTPUT / "behavior.ll"
    run(compiler, SOURCE, "-o", executable)
    assert run(executable).splitlines() == ["call effects 0"]
    run(compiler, "emit-llvm", SOURCE, "-o", llvm)
    ir = llvm.read_text(encoding="utf-8")
    for name in ("safe_queries", "safe_pop"):
        body = function_body(ir, name)
        assert "@txrt_gc_safepoint_context(" not in body, name
        assert "@txrt_value_clone(" not in body, name
        assert "@txrt_value_box_" not in body, name
    for name in ("unsafe_pop", "unsafe_contains", "text_query"):
        assert "@txrt_gc_safepoint_context(" in function_body(ir, name), name
    assert "@txrt_dictionary_contains_i64(" in function_body(ir, "safe_queries")
    print("PASS 调用效果：标量查询和 pop 无安全点；对象回调/析构保留安全点与错误")


if __name__ == "__main__":
    main()
