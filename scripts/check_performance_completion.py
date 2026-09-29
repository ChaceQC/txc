"""剩余优化的基线、定向验证与最终交替测量。"""

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import statistics
import re

root = Path(__file__).resolve().parents[1]
work = root / "tx_build/performance_completion"
archive = root / "benchmarks/performance_completion_2026-09-30"
sources = {"diverse": "benchmarks/diverse_performance.tx",
           "language": "benchmarks/language_features/compare.tx"}


def run(*args):
    result = subprocess.run([str(a) for a in args], cwd=root, capture_output=True,
                            encoding="utf-8", errors="strict", timeout=240)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    return result.stdout


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def save(name, value):
    archive.mkdir(parents=True, exist_ok=True)
    (archive / name).write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def build_programs(label):
    work.mkdir(parents=True, exist_ok=True)
    files = {}
    for name, relative in sources.items():
        source = root / relative
        executable = work / f"{name}_{label}.exe"
        llvm = work / f"{name}_{label}.ll"
        optimized = work / f"{name}_{label}_optimized.ll"
        run(root / "tx/txc.exe", source, "-o", executable)
        run(root / "tx/txc.exe", "emit-llvm", source, "-o", llvm)
        run(root / "tx/clang.exe", "--target=x86_64-w64-windows-gnu", "-O3", "-S", "-emit-llvm",
            "-x", "ir", llvm, "-o", optimized)
        files[name] = {"source": digest(source), "executable": digest(executable), "ir": digest(llvm)}
    save(f"{label}.json", {"head": run("git", "rev-parse", "HEAD").strip(), "files": files,
                           "compiler": digest(root / "tx/txc.exe"),
                           "stdlib": digest(root / "tx/libtxstdlib.a")})


def checks():
    cases = ["static_borrowing", "static_borrowing_lifetime", "performance_completion", "performance_completion_gc",
             "strings/format_optimized", "serde/behavior", "serde/direct",
             "performance_12_14/graphs", "performance_12_14/arithmetic",
             "performance_hotspots/virtual_views", "performance_hotspots/serde_wide",
             "bytes_file_stream/encodings", "crypto/behavior"]
    output = {}
    for case in cases:
        executable = work / (case.replace("/", "_") + ".exe")
        run(root / "tx/txc.exe", root / "tests" / (case + ".tx"), "-o", executable)
        output[case] = run(executable).strip()
        print(f"PASS {case}", flush=True)
    output["package"] = run("python", "-X", "utf8", "-B", root / "scripts/check_package_compatibility.py")
    native = work / "native_storage.exe"
    run("g++", "-std=c++23", "-O2", "-Isrc", root / "tests/performance_completion_native.cpp",
        root / "src/backend/cpp/typed_slots.cpp", "-o", native)
    output["native_storage"] = run(native).strip()
    save("checks.json", output)


def paired_commands(name, programs):
    samples = {label: {} for label in programs}
    expected = None
    for iteration in range(6):
        order = list(programs) if iteration % 2 == 0 else list(reversed(programs))
        for label in order:
            lines = run(*programs[label]).splitlines()
            values = {lines[i]: (float(lines[i + 1]) / 1000, lines[i + 2]) for i in range(0, len(lines), 3)}
            checksums = {key: value[1] for key, value in values.items()}
            expected = checksums if expected is None else expected
            assert checksums == expected
            if iteration:
                for key, (elapsed, _) in values.items():
                    samples[label].setdefault(key, []).append(elapsed)
    save(name + ".json", {"warmups": 1, "rounds": 5, "checksums": expected, "samples_ms": samples,
                          "medians_ms": {label: {key: statistics.median(value) for key, value in rows.items()}
                                         for label, rows in samples.items()},
                          "program_sha256": {label: digest(command[0]) for label, command in programs.items()}})


def contracts(graph=True, formatting=True):
    native_graph = work / "graph_contract.exe"
    graph_source = archive / "graph_contract.cpp"
    if graph:
        run("g++", "-std=c++23", "-O3", "-finput-charset=UTF-8", "-fexec-charset=UTF-8", "-Isrc",
            graph_source, root / "tx/libtxstdlib.a", "-Ltx/link", "-lwinhttp", "-lws2_32", "-ladvapi32",
            "-lbcrypt", "-lshell32", "-luser32", "-liconv", "-lpsapi", "-o", native_graph)
        paired_commands("graph_contract", {"tx_runtime": [native_graph], "cpp": [native_graph, "reference"]})
    if not formatting:
        return
    cpp_format = work / "format_cpp.exe"
    tx_format = work / "format_tx.exe"
    run("g++", "-std=c++23", "-O3", archive / "format_contract.cpp", "-o", cpp_format)
    run(root / "tx/txc.exe", archive / "format_contract.tx", "-o", tx_format)
    run(root / "tx/txc.exe", "emit-llvm", archive / "format_contract.tx", "-o", work / "format_contract.ll")
    ir = (work / "format_contract.ll").read_text(encoding="utf-8")
    assert ir.count("call i32 @txrt_format_arguments_context(") >= 2
    paired_commands("format_contract", {"tx": [tx_format], "cpp": [cpp_format]})


def measure():
    old = json.loads((archive / "old.json").read_text(encoding="utf-8"))
    for name, relative in sources.items():
        assert digest(work / f"{name}_old.exe") == old["files"][name]["executable"]
        assert digest(root / relative) == old["files"][name]["source"]
        print(run("python", "-X", "utf8", "-B", root / "scripts/compare_performance_programs.py",
                  "--old", work / f"{name}_old.exe", "--new", work / f"{name}_new.exe",
                  "--source", root / relative, "--output", archive / f"{name}.json", "--rounds", "5").strip())


def verify_ir():
    def body(ir, name, suffix=""):
        match = re.search(r"^define [^\n]*@tx_fn_[^(]*" + name + "_0" + suffix +
                          r"\([^\n]*\).*?^}", ir, re.M | re.S)
        assert match, name + suffix
        return match.group()
    observations = {}
    groups = {"language": ["bench_struct_operators", "bench_module_call", "bench_recursion", "bench_runtime_cast"],
              "diverse": ["bench_format_paths", "bench_encoding_paths", "bench_serde_size"]}
    symbols = ["txrt_record_struct_new", "txrt_value_clone", "txrt_record_class_view",
               "txrt_record_require_type", "txrt_format_begin", "txrt_format_execute",
               "txrt_encoding_encode_literal", "txrt_value_to_str", "txrt_gc_safepoint_context"]
    for group, names in groups.items():
        for label in ["old", "new"]:
            ir = (work / f"{group}_{label}_optimized.ll").read_text(encoding="utf-8")
            for name in names:
                code = body(ir, name)
                observations.setdefault(name, {})[label] = {symbol: code.count("@" + symbol + "(") for symbol in symbols}
                if label == "new" and name in ["bench_struct_operators", "bench_module_call"]:
                    assert "@txrt_record_struct_new(" not in code and "@txrt_value_clone(" not in code
                if label == "new" and name == "bench_format_paths":
                    assert "@txrt_format_execute(" in code and "@txrt_format_begin(" not in code
                if label == "new" and name == "bench_encoding_paths":
                    assert "@txrt_encoding_encode_literal(" in code
            if label == "new" and group == "diverse":
                assert "ptr @tx_serde_pair_i64_str" in ir
    raw = (work / "language_new.ll").read_text(encoding="utf-8")
    bounded = body(raw, "recursive_sum", "_bounded")
    assert "%tx_frame" not in bounded and "@txrt_add_i64(" not in bounded
    assert "@llvm.sadd.with.overflow" not in bounded
    assert "@tx_fn_m0_recursive_sum_0_bounded(" in body(raw, "bench_recursion")
    assert "@tx_class_view_destructors_" in raw
    save("ir_checks.json", observations)
    changed = set(run("git", "diff", "--name-only", "--", "src", "CMakeLists.txt").splitlines())
    changed.update(run("git", "ls-files", "--others", "--exclude-standard", "--", "src").splitlines())
    save("candidate.json", {"compiler": digest(root / "tx/txc.exe"), "stdlib": digest(root / "tx/libtxstdlib.a"),
                            "source_sha256": {name: digest(root / name) for name in sorted(changed)},
                            "target": "x86_64-w64-windows-gnu", "optimization": "-O3"})
    print("IR_CHECKS_OK", flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=["baseline", "build", "check", "measure", "contracts", "format-contract", "graph-contract", "ir"])
    mode = parser.parse_args().mode
    if mode == "baseline":
        assert not (archive / "old.json").exists(), "拒绝覆盖基线"
        build_programs("old")
    elif mode == "build":
        build_programs("new")
    elif mode == "check":
        checks()
    elif mode == "contracts":
        contracts()
    elif mode == "format-contract":
        contracts(False)
    elif mode == "graph-contract":
        contracts(True, False)
    elif mode == "ir":
        verify_ir()
    else:
        measure()
