"""只验证调用借用、键专用化及会使借用失效的语义边界。"""

from pathlib import Path
import re
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
COMPILER = ROOT / "tx" / "txc.exe"
SOURCE = ROOT / "tests" / "containers" / "call_borrowing.tx"
OUTPUT = ROOT / "tx_build" / "call_borrowing_checks"


def run(arguments):
    result = subprocess.run(
        [str(value) for value in arguments], cwd=ROOT, capture_output=True,
        encoding="utf-8", errors="strict", timeout=45,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


def function_body(ir, name):
    match = re.search(
        rf"^define [^\n]* @tx_fn_m0_{re.escape(name)}_0\([^\n]*\)[^\n]*\{{\n(.*?)^\}}",
        ir, re.MULTILINE | re.DOTALL,
    )
    assert match, f"缺少函数 {name}"
    return match.group(1)


def check_ir(ir):
    for name in ("query_map", "query_set", "query_queue", "query_length", "query_dictionary",
                 "query_named", "query_cancel"):
        body = function_body(ir, name)
        assert "call i32 @txrt_value_clone(" not in body, name
        assert "call i32 @txrt_value_box_" not in body, name
        assert "call void @txrt_value_release(" not in body, name
    for name in ("rebind_map", "rebind_dictionary", "callback_index"):
        assert "call i32 @txrt_value_clone(" in function_body(ir, name), name
    for suffix in ("i64", "f64", "bool"):
        for operation in ("get", "contains", "remove"):
            assert re.search(rf"call i32 @txrt_dictionary_{operation}_{suffix}\(", ir), (
                operation, suffix,
            )
    print("PASS IR：七个安全查询无克隆/装箱/释放；危险路径保留持有；九个标量键入口")


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    OUTPUT.mkdir(parents=True, exist_ok=True)
    executable = OUTPUT / "behavior.exe"
    llvm = OUTPUT / "behavior.ll"
    run([COMPILER, SOURCE, "-o", executable])
    result = run([executable])
    assert result.splitlines() == ["call borrowing 0"], result
    print("PASS 行为：重绑定、共享、混合键、NaN、叶子错误、取消、回调和析构")
    run([COMPILER, "emit-llvm", SOURCE, "-o", llvm])
    check_ir(llvm.read_text(encoding="utf-8"))


if __name__ == "__main__":
    main()
