"""小范围检查类型化容器：三个正常场景、七个运行错误、三个编译诊断。"""

from pathlib import Path
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "tx_build" / "typed_container_checks"
SOURCES = ROOT / "tests" / "containers"
COMPILER = ROOT / "tx" / "txc.exe"


def compile_source(source):
    target = OUTPUT / f"{source.stem}.exe"
    result = subprocess.run(
        [str(COMPILER), str(source), "-o", str(target)],
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
    result = run(compile_source(ROOT / "examples" / "typed_containers.tx"))
    assert result.returncode == 0, result.stderr
    assert result.stdout.splitlines() == [
        "3 0 true", "4 3", "true false", "上海", "[2, 5, 8]", "5", "8",
        "first second", "first", "second", "true 0", "4",
    ], result.stdout
    print("PASS 容器示例")
    return 1


def check_behavior():
    result = run(compile_source(SOURCES / "behavior.tx"))
    assert result.returncode == 0, result.stderr
    lines = result.stdout.splitlines()
    assert len(lines) == 4 and lines[0] == "maps true", result.stdout
    assert lines[2:] == ["fields true", "order true"], result.stdout
    assert 'map<str,str>{"中文": "内容!"}' in lines[1], lines[1]
    assert "set<bool>{" in lines[1] and "true" in lines[1] and "false" in lines[1], lines[1]
    assert "heap<float>(min)[-1.0, 2.5]" in lines[1], lines[1]
    assert 'queue<str>["内容!"]' in lines[1], lines[1]
    print("PASS 组合行为：模块、类型、字段、求值顺序、rehash、快照和对象图复制")
    return 1


def check_errors():
    target = compile_source(SOURCES / "errors.tx")
    messages = [
        "map 键不存在", "空 heap 不能 pop", "空 queue 不能 front",
        "NaN 不能用作", "NaN 不能用作", "NaN 不能用作", "类型不匹配：map<str,str>",
    ]
    for mode, message in enumerate(messages):
        result = run(target, f"{mode}\n")
        assert result.returncode == 1, (mode, result.returncode, result.stderr)
        assert result.stdout.splitlines() == ["still alive"], (mode, result.stdout)
        assert "运行错误" in result.stderr and message in result.stderr, (mode, result.stderr)
        print(f"PASS 运行错误和析构清理 {mode}: {message}")
    return len(messages)


def check_diagnostics():
    cases = {
        "wrong_key.tx": "map 键",
        "wrong_value.tx": "heap 操作参数数量或类型不匹配",
        "nested_type.tx": "map 当前支持 int、float、bool、str 类型参数",
    }
    for name, message in cases.items():
        result = subprocess.run(
            [str(COMPILER), "check", str(SOURCES / name)],
            capture_output=True, encoding="utf-8", errors="strict", timeout=10,
        )
        assert result.returncode == 1, (name, result.returncode, result.stderr)
        assert name in result.stderr and "错误" in result.stderr and message in result.stderr, result.stderr
        print(f"PASS 静态诊断: {name}")
    return len(cases)


def check_main_example():
    existing = set((ROOT / "tx_build").glob("example_interfaces_*"))
    result = run(compile_source(ROOT / "example.tx"))
    assert result.returncode == 0, result.stdout + result.stderr
    lines = result.stdout.splitlines()
    expected = [
        "类型化 map： 3 0 true", "map 共享与复制： 4 3", "set 去重： true false",
        "set 快照遍历： 上海", "小顶堆顺序： [2, 5, 8]", "删除后的堆顶： 5",
        "大顶堆堆顶： 8", "队首与队尾： first second", "FIFO 出队： first",
        "FIFO 出队： second", "队列已空： true 0", "any 恢复 map： 4",
    ]
    start = lines.index(expected[0])
    assert lines[start:start + len(expected)] == expected, result.stdout
    assert set((ROOT / "tx_build").glob("example_interfaces_*")) == existing
    print("PASS 根目录 example.tx：新增容器输出、原有主流程和示例目录清理")
    return 1


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    groups = {"example": check_example, "behavior": check_behavior,
              "errors": check_errors, "diagnostics": check_diagnostics,
              "main_example": check_main_example}
    selected = sys.argv[1:] or list(groups)
    if any(name not in groups for name in selected):
        raise SystemExit("可选分组：example behavior errors diagnostics main_example")
    OUTPUT.mkdir(parents=True, exist_ok=True)
    count = 0
    for name in selected:
        count += groups[name]()
    print(f"类型化容器定向验证：{count}/{count}")


if __name__ == "__main__":
    main()
