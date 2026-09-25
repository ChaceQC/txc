"""小范围验证新增容器和文件接口，共 7 个运行场景。"""

import ast
from pathlib import Path
import subprocess
import sys
import tempfile
import time


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "tx_build" / "stdlib_interface_checks"

sys.stdout.reconfigure(encoding="utf-8")


def compile_source(source):
    target = OUTPUT / f"{source.stem}.exe"
    result = subprocess.run(
        [str(ROOT / "tx" / "txc.exe"), str(source), "-o", str(target)],
        capture_output=True, encoding="utf-8", errors="strict", timeout=30,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    return target


def run(target, directory, mode=""):
    return subprocess.run(
        [str(target)], input=mode, cwd=directory, capture_output=True,
        encoding="utf-8", errors="strict", timeout=10,
    )


def array_text(text):
    return ast.literal_eval(text.replace("none", "None"))


def check_container_example(directory):
    result = run(compile_source(ROOT / "examples/container_interfaces.tx"), directory)
    assert result.returncode == 0, result.stderr
    lines = result.stdout.splitlines()
    assert len(lines) == 5, lines
    assert array_text(lines[0]) == [None, "二"], lines
    assert lines[1] == "空键", lines
    assert dict(array_text(lines[2])) == {None: "空键", "nested": [None, "二"]}, lines
    assert dict(array_text(lines[3])) == {None: "空键", "nested": []}, lines
    assert array_text(lines[4]) == [1, 2, 1, 2], lines
    print("PASS 容器示例：共享修改、条目快照、遍历期间追加")


def check_filesystem_example(directory):
    started = int(time.time() * 1000)
    result = run(compile_source(ROOT / "examples/filesystem_interfaces.tx"), directory)
    assert result.returncode == 0, result.stderr
    lines = result.stdout.splitlines()
    assert len(lines) == 9 and lines[0] == "7", lines
    assert started - 2000 <= int(lines[1]) <= int(time.time() * 1000) + 2000, lines
    assert array_text(lines[2]) == ["原文.txt", "子目录"], lines
    assert array_text(lines[3]) == ["原文.txt", "子目录", "子目录/移动.txt"], lines
    assert lines[4:6] == ["a/c.txt", "子目录/移动.txt"], lines
    assert lines[6].endswith("/子目录/移动.log"), lines
    assert lines[7:] == ["true", "3"], lines
    assert not (directory / Path(lines[6]).parents[1]).exists(), lines
    print("PASS 文件示例：中文路径、复制移动、元信息、列举和删除")


def check_boundaries(directory):
    target = compile_source(ROOT / "tests/stdlib/interfaces.tx")
    expected = [
        '[none, 2, 9, "三"]', "true", '[none, 2, 9, "三"]',
        "true", "false", "true", "[[none, []]]", '["released"]',
    ]
    result = run(target, directory, "0\n")
    assert result.returncode == 0, result.stderr
    assert result.stdout.splitlines() == expected, result.stdout
    print("PASS 组合边界：实参求值顺序、命名与展开调用、none 键、析构回调")
    for mode, message in (
        (1, "不能删除空数组的尾元素"), (2, "数组删除位置越界"),
        (3, "复制文件失败"), (4, "移动目标已存在"),
    ):
        result = run(target, directory, f"{mode}\n")
        assert result.returncode == 1 and message in result.stderr, (
            mode, result.returncode, result.stdout, result.stderr,
        )
        if mode >= 3:
            assert (directory / "source.txt").read_text(encoding="utf-8") == "source"
            assert (directory / "target.txt").read_text(encoding="utf-8") == "keep"
        print(f"PASS 预期错误：{message}")


def main():
    assert OUTPUT.resolve().is_relative_to(ROOT)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="run_", dir=OUTPUT) as temporary:
        directory = Path(temporary).resolve()
        assert directory.is_relative_to(OUTPUT.resolve())
        check_container_example(directory)
        check_filesystem_example(directory)
        check_boundaries(directory)
    print("标准库接口定向验证：7/7")


if __name__ == "__main__":
    main()
