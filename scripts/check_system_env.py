"""system/env 定向验证：2 个正常场景、4 个运行错误、1 个静态诊断。"""

import ast
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "tx_build" / "system_env_checks"
COMPILER = ROOT / "tx" / "txc.exe"
TEST_NAME = "TX_SYSTEM_ENV_TEST_中文"
TEST_VALUE = "继承的长值😀=" * 80

sys.stdout.reconfigure(encoding="utf-8")


def compile_source(source, target, environment=None):
    result = subprocess.run(
        [str(COMPILER), str(source), "-o", str(target)], capture_output=True,
        env=environment, timeout=30,
    )
    assert result.returncode == 0, (result.stdout + result.stderr).decode(
        "utf-8", errors="backslashreplace",
    )
    assert target.is_file(), target


def run(target, directory, environment, arguments=(), text=""):
    return subprocess.run(
        [str(target), *arguments], cwd=directory, env=environment, input=text,
        capture_output=True, encoding="utf-8", errors="strict", timeout=10,
    )


def check_paths(lines, directory, target):
    assert len(lines) == 4, lines
    assert all("\\" not in line and Path(line).is_absolute() for line in lines), lines
    assert Path(lines[0]).samefile(directory), lines[0]
    assert Path(lines[1]).samefile(target), lines[1]
    assert all(Path(line).is_dir() for line in lines[2:]), lines[2:]


def check_example(directory, environment):
    target = OUTPUT / "system_env_example.exe"
    compile_source(ROOT / "examples" / "system_env.tx", target)
    result = run(target, directory, environment)
    assert result.returncode == 0, result.stderr
    lines = result.stdout.splitlines()
    assert len(lines) == 6 and lines[0] == "程序参数： []", lines
    check_paths([line.split("： ", 1)[1] for line in lines[1:5]], directory, target)
    assert lines[5] == "示例标签： 专用测试", lines[5]
    print("PASS 无参数示例：空向量、工作目录、可执行路径、临时目录和主目录")


def check_values(target, directory, environment):
    arguments = ["输入 文件.txt", "", "中文😀", '带"引号', "尾空格 ", "尾\\", "空 格\\"]
    result = run(target, directory, environment, arguments, "0\n")
    assert result.returncode == 0, result.stderr
    lines = result.stdout.splitlines()
    assert len(lines) == 14, lines
    assert ast.literal_eval(lines[0]) == arguments, lines[0]
    assert ast.literal_eval(lines[1]) == arguments, lines[1]
    check_paths(lines[2:6], directory, target)
    assert lines[6:] == [
        "true true true", "true", "true true", TEST_VALUE, "true true true",
        "更新=值😀", "true false false", "未设置",
    ], lines[6:]
    assert (directory / "子目录" / "位置.txt").read_text(encoding="utf-8") == "子目录"
    assert not (directory / "位置.txt").exists()
    assert environment[TEST_NAME] == TEST_VALUE
    print("PASS 参数与状态：Unicode、空值和引号、快照隔离、切换目录、环境变量增删改查")


def check_errors(target, directory, environment):
    for mode, text, message in (
        (1, "", "环境变量未设置"),
        (2, "", "环境变量名称不能为空或包含等号、NUL 字符"),
        (3, "不可截断\0尾部\n", "环境变量的值不能包含 NUL 字符"),
        (4, "", "切换工作目录失败"),
    ):
        result = run(target, directory, environment, text=f"{mode}\n{text}")
        assert result.returncode == 1 and message in result.stderr, (
            mode, result.returncode, result.stdout, result.stderr,
        )
        assert not result.stdout, result.stdout
        print(f"PASS 预期错误：{message}")


def check_types():
    source = ROOT / "tests" / "stdlib" / "system_env_types.tx"
    result = subprocess.run(
        [str(COMPILER), "check", str(source)], capture_output=True,
        encoding="utf-8", errors="strict", timeout=10,
    )
    assert result.returncode != 0, result.stdout
    assert "没有匹配的函数重载" in result.stderr, result.stderr
    assert re.search(r"system_env_types\.tx:\d+:\d+:", result.stderr), result.stderr
    print("PASS 静态诊断：默认值类型错误包含源码位置")


def main():
    assert OUTPUT.resolve().is_relative_to(ROOT)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    environment[TEST_NAME] = TEST_VALUE
    environment["TX_SYSTEM_ENV_EXAMPLE_LABEL"] = "专用测试"
    target = OUTPUT / "中文 输出" / "中文 程序.exe"
    # 同时覆盖 clang 的临时输入/输出路径和链接器的最终输出路径。
    with tempfile.TemporaryDirectory(prefix="编译 临时_", dir=OUTPUT) as temporary:
        compiler_environment = environment.copy()
        compiler_environment["TMP"] = temporary
        compiler_environment["TEMP"] = temporary
        compile_source(ROOT / "tests" / "stdlib" / "system_env.tx", target,
                       compiler_environment)
    # 测试只打印自建变量；环境修改发生在子进程，测试目录退出时清理。
    with tempfile.TemporaryDirectory(prefix="工作目录_", dir=OUTPUT) as temporary:
        directory = Path(temporary).resolve()
        assert directory.is_relative_to(OUTPUT.resolve())
        (directory / "子目录").mkdir()
        check_example(directory, environment)
        check_values(target, directory, environment)
        check_errors(target, directory, environment)
        check_types()
    print("system/env 定向验证：7/7")


if __name__ == "__main__":
    main()
