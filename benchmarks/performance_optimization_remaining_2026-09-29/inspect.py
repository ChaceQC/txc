"""保存最终调用者和运行时热路径；不运行负载。"""
import json
import hashlib
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[2]
archive = Path(__file__).resolve().parent
work = root / "tx_build/performance_remaining"


def disassemble(program, symbol):
    process = subprocess.run(["objdump", "-d", "--disassemble=" + symbol, str(program)],
                             capture_output=True, encoding="utf-8", errors="strict", check=True)
    assert "<" + symbol + ">:" in process.stdout, symbol
    return process.stdout


def main():
    report = {}
    for label, directory in (("old", "baseline"), ("new", "candidate")):
        targets = {
            "borrowing": ["tx_fn_m0_bench_map_0", "txrt_map_read_i64_i64"],
            "diverse": ["tx_fn_m0_bench_parse_paths_0", "txrt_parse_int_scalar" + ("_context" if label == "new" else "")],
        }
        if label == "new":
            targets["paths"] = ["_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE",
                                "_ZN2tx6detail14scan_utf8_bulkESt17basic_string_viewIcSt11char_traitsIcEE"]
        sections = []
        for name, symbols in targets.items():
            for symbol in symbols:
                assembly = disassemble(work / directory / (name + ".exe"), symbol)
                sections.append(assembly)
                report[label + ":" + symbol] = {
                    "calls": re.findall(r"\bcall\s+[^\n]+", assembly),
                    "vector_instruction_count": len(re.findall(r"%xmm\d+", assembly)),
                    "division_count": len(re.findall(r"\bdiv\s", assembly)),
                }
        (archive / (label + "_hotpaths.asm")).write_text("\n".join(sections), encoding="utf-8")
    (archive / "machine.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    sources = ["CMakeLists.txt", "scripts/build.ps1", "scripts/check_parse_scalar.py",
               "scripts/check_performance_remaining.py", "src/backend/cpp/parse_abi.cpp",
               "src/backend/cpp/parse_abi.hpp", "src/backend/llvm/codegen_declarations.cpp",
               "src/backend/llvm/codegen_native_parse.cpp", "src/common/utf8.hpp",
               "src/common/utf8.cpp", "src/common/utf8_bulk.cpp", "src/stdlib/container_scalar.hpp",
               "src/stdlib/typed_map.hpp", "src/stdlib/encoding.cpp", "src/stdlib/string.cpp",
               "src/stdlib/parse.cpp", "src/stdlib/bytes.cpp", "docs/performance_remaining.md",
               "docs/performance_optimization_plan.md", "docs/usage.md"]
    paths = [root / name for name in sources] + list((root / "tests/performance_remaining").glob("*")) + list(archive.glob("*.py"))
    manifest = {str(path.relative_to(root)): hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}
    (archive / "source_manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print("Saved old/new assembly, call summary and source manifest")


if __name__ == "__main__":
    main()
