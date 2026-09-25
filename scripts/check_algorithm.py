"""algorithm 的 8 个定向场景，包含根目录 example.tx，不运行其他回归。"""

from pathlib import Path
import re
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "tx_build" / "algorithm_checks"
SOURCES = ROOT / "tests" / "algorithm"
COMPILER = ROOT / "tx" / "txc.exe"


def compile_source(source):
    target = OUTPUT / f"{source.stem}.exe"
    result = subprocess.run(
        [str(COMPILER), str(source), "-o", str(target)], cwd=ROOT,
        capture_output=True, encoding="utf-8", errors="strict", timeout=30,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    return target


def run(target, mode=""):
    return subprocess.run(
        [str(target)], input=mode, cwd=ROOT, capture_output=True,
        encoding="utf-8", errors="strict", timeout=10,
    )


def check_example():
    result = run(compile_source(ROOT / "examples" / "algorithm.tx"))
    assert result.returncode == 0, result.stdout + result.stderr
    assert result.stdout.splitlines() == [
        "[4, 1, 4, 2] [1, 2, 4, 4]", "[1, 2, 4, 4] 2 2", "2 4", "11 1 4",
        "[4, 4, 2, 1] [1, 2, 4, 4]", "[-1.0, 0.0, 2.5, 2.5] 4.0", "-1.0 2.5",
        '["梨", "apple", "香蕉", "apple"] ["apple", "apple", "梨", "香蕉"]',
        "0 2", "0 2", "[false, false, true] 2 2", '["alice", "bob"] 175',
        "[2, 5]", "[1, 3] 3", "[7, 2] 2",
    ], result.stdout
    print("PASS algorithm 示例：四种元素与 map/set/queue/heap 快照")
    return 1


def check_behavior():
    result = run(compile_source(SOURCES / "behavior.tx"))
    assert result.returncode == 0, result.stdout + result.stderr
    assert result.stdout.splitlines() == [
        "empty and integers true", "float order true",
        "string ownership true", "argument order true",
    ], result.stdout
    print("PASS 综合边界：空向量、整数极值、NaN/零、字符串所有权与参数绑定")
    return 1


def check_errors():
    target = compile_source(SOURCES / "errors.tx")
    messages = [
        "algorithm.min_element 不能用于空 vector",
        "algorithm.sum 的结果超出 int 范围",
        "algorithm.sum 的结果不是有限 float",
        "algorithm.max_element 需要有限 float 元素",
    ]
    for mode, message in enumerate(messages):
        result = run(target, f"{mode}\n")
        assert result.returncode == 1, (mode, result.returncode, result.stderr)
        assert result.stdout.splitlines() == ["algorithm cleanup"], result.stdout
        assert "运行错误" in result.stderr and message in result.stderr, result.stderr
        print(f"PASS 运行错误与析构清理：{message}")
    return len(messages)


def check_diagnostic():
    result = subprocess.run(
        [str(COMPILER), "check", str(SOURCES / "wrong_type.tx")], cwd=ROOT,
        capture_output=True, encoding="utf-8", errors="strict", timeout=10,
    )
    assert result.returncode == 1, result.stdout + result.stderr
    assert re.search(r"wrong_type\.tx:\d+:\d+: 错误", result.stderr), result.stderr
    assert "find" in result.stderr and "匹配" in result.stderr, result.stderr
    print("PASS 静态诊断：vector<int> 查找 float 被拒绝并报告源码位置")
    return 1


def check_main_example():
    existing = set((ROOT / "tx_build").glob("example_interfaces_*"))
    result = run(compile_source(ROOT / "example.tx"))
    assert result.returncode == 0, result.stdout + result.stderr
    lines = result.stdout.splitlines()
    expected = [
        "排序副本： [4, 1, 4, 2] [1, 2, 4, 4]", "原地排序： [1, 2, 4, 4]",
        "查找与计数： 2 2", "二分边界： 2 4", "整数统计： 11 1 4",
        "原地反转： [4, 4, 2, 1] [1, 2, 4, 4]", "浮点统计： 4.0 -1.0 2.5",
        '字符串排序： ["apple", "apple", "梨", "香蕉"]',
        "布尔查找： [false, false, true] 2 2", 'map 快照算法： ["alice", "bob"] 175',
    ]
    start = lines.index(expected[0])
    assert lines[start:start + len(expected)] == expected, result.stdout
    assert lines[-1] == "结束", result.stdout
    assert set((ROOT / "tx_build").glob("example_interfaces_*")) == existing
    print("PASS 根目录 example.tx：算法展示、原有主流程和示例目录清理")
    return 1


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    groups = {
        "example": check_example, "main_example": check_main_example,
        "behavior": check_behavior, "errors": check_errors,
        "diagnostic": check_diagnostic,
    }
    selected = sys.argv[1:] or list(groups)
    if any(name not in groups for name in selected):
        raise SystemExit("可选分组：example main_example behavior errors diagnostic")
    OUTPUT.mkdir(parents=True, exist_ok=True)
    count = sum(groups[name]() for name in selected)
    print(f"algorithm 定向验证：{count}/{count}")


if __name__ == "__main__":
    main()
