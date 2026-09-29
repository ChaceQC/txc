"""06 的局部结果身份、借用边界、整数核心与优化 IR 定向检查。"""

import json
from pathlib import Path
import re
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
work = root / "tx_build/parse_scalar_checks"
archive = root / "benchmarks/performance_optimization_06_2026-09-29"
sys.stdout.reconfigure(encoding="utf-8")


def run(arguments):
    result = subprocess.run([str(arg) for arg in arguments], cwd=root,
                            capture_output=True, encoding="utf-8", errors="strict", timeout=180)
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


def main():
    work.mkdir(parents=True, exist_ok=True)
    compiler = root / "tx/txc.exe"
    behavior = work / "behavior.exe"
    run([compiler, root / "tests/stdlib/parse_scalar.tx", "-o", behavior])
    assert run([behavior]).splitlines() == ["PARSE_SCALAR_OK"]
    core = work / "core.exe"
    run(["g++", "-std=c++23", "-O2", "-I", root / "src",
         root / "tests/stdlib/parse_scalar.cpp", root / "src/stdlib/parse.cpp", "-o", core])
    assert run([core]).splitlines() == ["PARSE_CORE_OK"]
    raw = work / "parse.ll"
    optimized = work / "parse_optimized.ll"
    run([compiler, "emit-llvm", archive / "parse_paths.tx", "-o", raw])
    run([root / "tx/clang.exe", "-target", "x86_64-w64-windows-gnu",
         "-O3", "-S", "-emit-llvm", raw, "-o", optimized])
    ir = optimized.read_text(encoding="utf-8")
    local = re.search(r"define[^\n]*@tx_fn_m0_local_ok_0\([^\n]*\).*?\n}", ir, re.S)
    assert local, "local_ok function missing"
    calls = sorted(set(re.findall(r"call[^\n]*?@(\w+)\(", local.group())))
    assert "txrt_parse_int_scalar_context" in calls, calls
    forbidden = [name for name in calls if any(token in name for token in
        ["parse_materialize", "parse_try_parse", "str_clone", "gc_safepoint", "struct_", "value_clone", "value_release"])]
    assert not forbidden, forbidden
    full = re.search(r"define[^\n]*@tx_fn_m0_full_result_0\([^\n]*\).*?\n}", ir, re.S)
    assert full and "@txrt_parse_materialize_int" in full.group()
    evidence = {"target": "x86_64-w64-windows-gnu", "optimization": "-O3",
                "behavior": "PARSE_SCALAR_OK", "native": "PARSE_CORE_OK",
                "local_ok_calls": calls, "full_result_materializes": True}
    (archive / "structure.json").write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print("PASS 局部结果身份、可写字段、异常合流、借用和兼容调用")
    print("PASS 2~36 进制及错误优先级，标量核心成功/失败零分配")
    print("PASS 优化 IR：local_ok 无结果/错误对象、clone、release 或安全点")


if __name__ == "__main__":
    main()
