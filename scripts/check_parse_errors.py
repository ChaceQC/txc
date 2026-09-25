"""parse 与异常的六个定向场景，不执行其他回归脚本。"""

from pathlib import Path
import re
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "tx_build" / "parse_error_checks"
COMPILER = ROOT / "tx" / "txc.exe"

sys.stdout.reconfigure(encoding="utf-8")


def execute(arguments, **options):
    return subprocess.run(
        [str(value) for value in arguments], capture_output=True,
        encoding="utf-8", errors="strict", timeout=60, **options,
    )


def compile_source(source, name):
    target = OUTPUT / f"{name}.exe"
    result = execute([COMPILER, ROOT / source, "-o", target])
    assert result.returncode == 0, result.stdout + result.stderr
    return target


def check_diagnostic(name, message):
    source = ROOT / "tests" / "stdlib" / name
    result = execute([COMPILER, "check", source])
    assert result.returncode != 0 and message in result.stderr, result.stderr
    assert re.search(re.escape(name) + r":\d+:\d+:", result.stderr), result.stderr
    print(f"PASS 静态诊断：{message}")


def main():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    target = compile_source("tests/stdlib/parse_errors.tx", "behavior")
    with tempfile.TemporaryDirectory(prefix="工作目录_", dir=OUTPUT) as temporary:
        directory = Path(temporary).resolve()
        assert directory.is_relative_to(OUTPUT.resolve())
        result = execute([target], cwd=directory)
        assert result.returncode == 0, result.stdout + result.stderr
        assert result.stdout.splitlines() == ["PARSE_ERRORS_OK"], result.stdout
        assert not result.stderr, result.stderr
        assert not list(directory.iterdir()), list(directory.iterdir())
        print("PASS 解析边界、统一结果、跨模块与嵌套异常、快速调用及对象清理")

        result = execute([target, "unhandled"], cwd=directory)
        assert result.returncode == 1, (result.returncode, result.stdout, result.stderr)
        assert result.stdout.splitlines() == ["UNHANDLED_CLEANUP"], result.stdout
        assert "运行错误：" in result.stderr and "整数文本" in result.stderr, result.stderr
        print("PASS 未捕获异常：退出码 1、中文错误及析构")

        example = compile_source("examples/parse_errors.tx", "example")
        result = execute([example], cwd=directory)
        assert result.returncode == 0 and not result.stderr, result.stdout + result.stderr
        assert result.stdout.splitlines()[0:2] == ["-127", "true 125.0"], result.stdout
        assert "parse_error invalid_syntax" in result.stdout, result.stdout
        assert result.stdout.endswith("错误处理后继续执行\n"), result.stdout
        print("PASS 独立 parse 示例")

        main_example = compile_source("example.tx", "main_example")
        result = execute([main_example], cwd=directory)
        assert result.returncode == 0 and not result.stderr, result.stdout + result.stderr
        assert "解析结果 127" in result.stdout, result.stdout
        assert "已处理解析错误 invalid_syntax" in result.stdout, result.stdout
        assert "异常处理后继续" in result.stdout, result.stdout
        assert not list(directory.rglob("*.txt")), list(directory.rglob("*.txt"))
        print("PASS 主示例兼容性及新增演示")

    check_diagnostic("parse_invalid_handler.tx", "exception 需要 error.txh")
    check_diagnostic("parse_duplicate_handler.tx", "重复的 exception 错误类型")
    print("parse 与错误处理定向验证：6/6")


if __name__ == "__main__":
    main()
