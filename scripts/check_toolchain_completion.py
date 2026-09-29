"""第十三部分工具链的聚焦验证；不运行全库测试。"""

from pathlib import Path
import json
import subprocess
import tempfile
import sys

ROOT = Path(__file__).resolve().parent.parent
TXC = ROOT / "tx" / "txc.exe"


def command(*args: str | Path, expected: int = 0) -> subprocess.CompletedProcess:
    result = subprocess.run([str(TXC), *(str(arg) for arg in args)], cwd=ROOT,
                            capture_output=True, text=True, encoding="utf-8",
                            errors="replace", timeout=90)
    assert result.returncode == expected, (args, result.returncode, result.stdout, result.stderr)
    return result


def run_suite(path: Path, expected: int = 0, *options: str) -> dict:
    return json.loads(command("test", path, "--format", "json", *options,
                              expected=expected).stdout)


def main() -> None:
    for name in (sys.argv[1:] or ("basic_test.tx", "property_test.tx", "log_extended_test.tx", "profile_test.tx")):
        report = run_suite(ROOT / "tests" / "diagnostics" / name)
        assert report["summary"]["passed"] == 1, report
        print(f"通过：{name}")

    failure = ROOT / "tests" / "diagnostics" / "property_failure.tx"
    first = run_suite(failure, 1)
    second = run_suite(failure, 1)
    event = json.loads(first["cases"][0]["output"].splitlines()[0])
    assert event == json.loads(second["cases"][0]["output"].splitlines()[0])
    assert (event["seed"], event["original"], event["counterexample"]) == (100, 100, 12)
    print("通过：失败种子、反例缩减与退出码")
    fixture = run_suite(ROOT / "tests/diagnostics/fixture_failure.tx", 1)
    output = fixture["cases"][0]["output"]
    assert "teardown-ran" in output and "operation-must-not-run" not in output
    assert "fixture_failure.tx" in output and "assertion_failed" in output
    print("通过：夹具初始化失败仍执行清理，保留源码位置")

    with tempfile.TemporaryDirectory(prefix="tx_toolchain_") as directory:
        root = Path(directory)
        sources = {
            "a_test.tx": 'import "file.txh" as file\nimport "fs.txh" as fs\n'
                         'def main() -> int\n{\n    if fs.exists("owned.txt")\n'
                         '    {\n        return 1\n    }\n'
                         '    file.write_text("owned.txt", "isolated", "utf-8")\n    return 0\n}\n',
            "b_test.tx": 'import "time.txh" as time\ndef main() -> int\n'
                         '{\n    time.sleep_millis(10000)\n    return 0\n}\n',
            "c_test.tx": 'def main() -> int\n{\n    return "wrong"\n}\n',
        }
        sources["d_test.tx"] = sources["a_test.tx"]
        for name, source in sources.items():
            (root / name).write_text(source, encoding="utf-8")
        report = run_suite(root, 1, "--jobs", "3", "--timeout-ms", "2000")
        assert [case["name"] for case in report["cases"]] == sorted(sources), report
        assert [case["status"] for case in report["cases"]] == [
            "passed", "timeout", "compile_error", "passed"], report
        assert report["summary"]["timeouts"] == 1 and not (root / "owned.txt").exists()
        for options in (("--jobs", "0"), ("--timeout-ms", "-1"), ("--isolation", "invalid")):
            command("test", root, *options, expected=2)
        print("通过：并行稳定报告、超时、工作目录隔离、编译失败分类与参数校验")

        output = root / "profile.json"
        command("profile", ROOT / "examples" / "profile_workload.tx", "--warmup", "1",
                "--samples", "2", "--interval-ms", "2", "-o", output)
        profile = json.loads(output.read_text(encoding="utf-8"))
        assert profile["success"] and len(profile["runs"]) == 4, profile
        samples = [json.loads(run["profile_json"]) for run in profile["runs"] if run["instrumented"]]
        assert all(sample["cpu"] and sample["allocated_bytes"] > 0 for sample in samples)
        assert any(row["file"].endswith("profile_workload.tx")
                   for sample in samples for row in sample["cpu"]), samples
        print("通过：CPU/内存源码归因、热身、普通与分析样本对照")


if __name__ == "__main__":
    main()
