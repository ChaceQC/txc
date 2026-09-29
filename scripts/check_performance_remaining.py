"""剩余热点的行为、核心边界、生成代码及包兼容定向检查。"""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
work = root / "tx_build/performance_remaining/checks"
archive = root / "benchmarks/performance_optimization_remaining_2026-09-29"


def run(*args):
    environment = os.environ.copy()
    environment["PATH"] = str(root / "tx") + os.pathsep + environment["PATH"]
    process = subprocess.run([str(arg) for arg in args], cwd=root, env=environment,
                             capture_output=True, encoding="utf-8", errors="strict", timeout=120)
    assert process.returncode == 0, (args, process.stdout, process.stderr)
    return process.stdout


def check_behavior():
    work.mkdir(parents=True, exist_ok=True)
    results = {}
    cases = ("performance_remaining/behavior", "containers/map_extended", "containers/behavior") if "--containers-only" in sys.argv else (
        "performance_remaining/behavior", "stdlib/parse_scalar", "containers/map_extended",
        "bytes_file_stream/encodings", "performance_15_16/libraries")
    if "--text-only" in sys.argv:
        cases = ("performance_remaining/behavior", "bytes_file_stream/encodings", "performance_15_16/libraries")
    for name in cases:
        source = root / "tests" / (name + ".tx")
        target = work / (name.replace("/", "_") + ".exe")
        run(root / "tx/txc.exe", source, "-o", target)
        results[name] = {"output": run(target), "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest()}
        if name == "containers/behavior":
            checks = [line for line in results[name]["output"].splitlines()
                      if line.startswith(("maps ", "fields ", "order "))]
            assert checks == ["maps true", "fields true", "order true"], results[name]["output"]
        print("PASS", name, flush=True)
    if "--containers-only" in sys.argv:
        (archive / "container_checks.json").write_text(json.dumps(results, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        return results
    target = work / "native.exe"
    run("g++", "-std=c++23", "-O3", "-Isrc", "-finput-charset=UTF-8", "-fexec-charset=UTF-8",
        root / "tests/performance_remaining/native.cpp", root / "tx/libtxstdlib.a",
        "-Ltx/link", "-lwinhttp", "-lws2_32", "-ladvapi32", "-lbcrypt", "-lshell32",
        "-luser32", "-liconv", "-o", target)
    results["native"] = run(target)
    assert results["native"].strip() == "REMAINING_NATIVE_OK"
    print("PASS UTF-8 differential boundaries and explicit parse context", flush=True)
    if "--text-only" in sys.argv:
        (archive / "text_checks.json").write_text(json.dumps(results, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        return results
    target = work / "parse_core.exe"
    run("g++", "-std=c++23", "-O3", "-Isrc", "-finput-charset=UTF-8", "-fexec-charset=UTF-8",
        root / "tests/stdlib/parse_scalar.cpp", root / "src/stdlib/parse.cpp", "-o", target)
    results["parse_core"] = run(target)
    assert results["parse_core"].strip() == "PARSE_CORE_OK"
    print("PASS parse bases, limits, precedence and zero allocation", flush=True)
    raw = work / "parse.ll"
    optimized = work / "parse_optimized.ll"
    run(root / "tx/txc.exe", "emit-llvm",
        root / "benchmarks/performance_optimization_06_2026-09-29/parse_paths.tx", "-o", raw)
    run(root / "tx/clang.exe", "-target", "x86_64-w64-windows-gnu", "-O3", "-S", "-emit-llvm", raw, "-o", optimized)
    ir = optimized.read_text(encoding="utf-8")
    local = re.search(r"define[^\n]*@tx_fn_m0_local_ok_0\([^\n]*\).*?\n}", ir, re.S)
    assert local and "@txrt_parse_int_scalar_context(ptr" in local.group()
    calls = sorted(set(re.findall(r"call[^\n]*?@(\w+)\(", local.group())))
    assert not any(any(token in call for token in ("parse_materialize", "str_clone", "value_clone", "gc_safepoint")) for call in calls)
    results["local_parse_calls"] = calls
    results["parse_errors"] = run("python", "-X", "utf8", root / "scripts/check_parse_errors.py")
    (archive / "behavior_checks.json").write_text(json.dumps(results, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    return results


def main():
    work.mkdir(parents=True, exist_ok=True)
    results = {} if "--package-only" in sys.argv else check_behavior()
    results["compatibility"] = run("python", "-X", "utf8", root / "scripts/check_package_compatibility.py")
    members = run("ar", "t", root / "tx/libtxstdlib.a").splitlines()
    results["bridges"] = {}
    extracted = work / "bridges"
    extracted.mkdir(exist_ok=True)
    for name in ("httpx_bridge", "websocket_bridge", "requests_bridge"):
        assert members.count(name + ".o") == 1, name
        # MinGW ar p 的 stdout 会转换 LF，必须直接提取二进制文件再比较。
        subprocess.run(["ar", "x", str(root / "tx/libtxstdlib.a"), name + ".o"], cwd=extracted, check=True)
        data = (extracted / (name + ".o")).read_bytes()
        assert data == (root / "build" / (name + ".o")).read_bytes(), name
        results["bridges"][name] = hashlib.sha256(data).hexdigest()
    results["artifacts"] = {name: hashlib.sha256((root / "tx" / name).read_bytes()).hexdigest()
                            for name in ("txc.exe", "libtxstdlib.a", "package.compat")}
    filename = "package_checks.json" if "--package-only" in sys.argv else "checks.json"
    (archive / filename).write_text(json.dumps(results, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print("PASS package compatibility and three bridge objects", flush=True)


if __name__ == "__main__":
    main()
