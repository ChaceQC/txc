"""仅验证 vector.empty 补齐：两个正常场景和两个编译诊断。"""

from pathlib import Path
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
COMPILER = ROOT / "tx" / "txc.exe"
OUTPUT = ROOT / "tx_build" / "vector_empty_checks"
SOURCES = ROOT / "tests" / "containers"


def invoke(arguments, timeout=30):
    return subprocess.run(
        [str(argument) for argument in arguments], cwd=ROOT,
        capture_output=True, encoding="utf-8", errors="strict", timeout=timeout,
    )


def check_program(source, expected):
    target = OUTPUT / f"{source.stem}.exe"
    compiled = invoke([COMPILER, source, "-o", target])
    assert compiled.returncode == 0, compiled.stdout + compiled.stderr
    result = invoke([target], timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr
    assert not result.stderr, result.stderr
    assert result.stdout.splitlines() == expected, result.stdout
    print(f"PASS {source.name}")


def check_diagnostic(name, messages):
    result = invoke([COMPILER, "check", SOURCES / name], timeout=10)
    assert result.returncode == 1, (name, result.returncode, result.stderr)
    assert name in result.stderr and "错误" in result.stderr, result.stderr
    assert all(message in result.stderr for message in messages), result.stderr
    print(f"PASS 静态诊断 {name}")


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    OUTPUT.mkdir(parents=True, exist_ok=True)
    check_program(SOURCES / "vector_empty.tx", [
        "constructors true", "occupied true", "cleared true", "ownership true",
        "reuse true", "fields true", "receiver true",
    ])
    check_program(ROOT / "examples" / "typed_vectors.tx", [
        "7", "alpha/beta/gamma", "alpha", "0", "7", "9", "false",
        "true true false", "true true true", "8", "8 12", "5", "4", "1",
    ])
    check_diagnostic("vector_empty_argument.tx", ["vector 操作参数数量或类型不匹配：empty"])
    check_diagnostic("vector_empty_result.tx", ["int", "bool"])
    print("vector.empty 定向验证：4/4")


if __name__ == "__main__":
    main()
