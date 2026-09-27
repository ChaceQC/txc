"""验证上下文 ABI 的调用边界与诊断语义，不运行完整测试集。"""

from pathlib import Path
import re
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]
COMPILER = ROOT / "tx" / "txc.exe"
OUTPUT = ROOT / "tx_build" / "runtime_context_checks"


def execute(arguments, *, cwd=ROOT):
    return subprocess.run(
        [str(value) for value in arguments], cwd=cwd, capture_output=True,
        encoding="utf-8", errors="strict", timeout=60,
    )


def run_case(source, expected, *, exit_code=0, arguments=()):
    executable = OUTPUT / (Path(source).stem + ".exe")
    compiled = execute([COMPILER, ROOT / source, "-o", executable])
    assert compiled.returncode == 0, compiled.stdout + compiled.stderr
    with tempfile.TemporaryDirectory(prefix="case_", dir=OUTPUT) as directory:
        result = execute([executable, *arguments], cwd=directory)
    assert result.returncode == exit_code, result.stdout + result.stderr
    if expected is not None:
        assert result.stdout.splitlines() == expected, result.stdout
    if exit_code == 0:
        assert not result.stderr, result.stderr
    print(f"PASS {source}")
    return result


def check_stack():
    result = run_case("tests/stdlib/stack_trace.tx", None)
    lines = result.stdout.splitlines()
    assert lines[0] == "operation_failed", lines
    source = (ROOT / "tests/stdlib/stack_trace.tx").read_text(encoding="utf-8")
    statements = ("print(middle())", "return inner()", "return 1 / 0")
    expected = []
    for name, statement in zip(("main", "middle", "inner"), statements):
        row = next(index for index, line in enumerate(source.splitlines(), 1)
                   if statement in line)
        expected.append((name, row))
    actual = [re.fullmatch(r"(\w+) \(.*stack_trace.tx:(\d+):(\d+)\)", line)
              for line in lines[1:]]
    assert all(actual), lines
    assert [(item[1], int(item[2])) for item in actual] == expected, lines


def check_generated_calls():
    target = OUTPUT / "runtime_context.ll"
    result = execute([COMPILER, "emit-llvm",
                      ROOT / "tests/diagnostics/runtime_context.tx", "-o", target])
    assert result.returncode == 0, result.stdout + result.stderr
    ir = target.read_text(encoding="utf-8")
    assert not re.search(r"call .*@txrt_(stack_push|stack_pop|stack_location|error_status)\(", ir)
    assert "call i32 @txrt_random_int_context(ptr %tx_context," in ir
    assert "call i32 @txrt_random_float_context(ptr %tx_context," in ir
    functions = re.findall(r"define [^\n]+ @tx_fn_[^\n]+\n.*?^}", ir, re.M | re.S)
    assert functions
    for function in functions:
        assert "(ptr %tx_context" in function.splitlines()[0]
        assert "call ptr @txrt_runtime_context()" not in function
    print("PASS 内部函数传递上下文，诊断操作及错误检查不再查询 TLS")


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    OUTPUT.mkdir(parents=True, exist_ok=True)
    run_case("tests/diagnostics/runtime_context.tx", ["RUNTIME_CONTEXT_OK"])
    run_case("tests/stdlib/parse_errors.tx", ["PARSE_ERRORS_OK"])
    run_case("tests/stdlib/closure_error.tx", ["runtime_error operation_failed"])
    run_case("tests/stdlib/closure_cycle.tx", ["6"])
    run_case("examples/cycle_resurrection.tx", ["victim deinit", "7"])
    failure = run_case("tests/stdlib/hash_key_error.tx", [], exit_code=1)
    assert "hash_key" in failure.stderr and "运行错误：" in failure.stderr, failure.stderr
    check_stack()
    check_generated_calls()


if __name__ == "__main__":
    main()
