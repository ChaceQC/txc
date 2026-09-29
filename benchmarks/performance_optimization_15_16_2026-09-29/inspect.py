"""记录最终机器码、桥接对象、公开包身份，以及遗留 map 观察。"""
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

from measure import archive, digest, out, root, run, sample, save


def assembly(program, symbol):
    text, _ = run("objdump", "-d", "--no-show-raw-insn", "--disassemble=" + symbol, program)
    assert "<" + symbol + ">:" in text, symbol
    return text


def signature(text):
    instructions = []
    for line in text.splitlines():
        match = re.match(r"\s*[0-9a-f]+:\s+(.*)", line)
        if match:
            instruction = match[1].split("#")[0].strip()
            instruction = re.sub(r"\b[0-9a-f]{8,}\s+(<[^>]+>)", r"\1", instruction)
            instruction = re.sub(r"-?0x[0-9a-f]+\(%rip\)", "relocation(%rip)", instruction)
            instructions.append(instruction)
    return hashlib.sha256("\n".join(instructions).encode()).hexdigest()


def inspect_code():
    hex_symbol = "_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE"
    unhex_symbol = "_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE"
    calls = {}
    for label, folder in (("old", "baseline"), ("new", "candidate")):
        parts = []
        calls[label] = {}
        selected = {
            "paths": [hex_symbol, unhex_symbol, "tx_fn_m0_hex_paths_0",
                      "tx_fn_m0_encoding_paths_0", "txrt_statistics_mean_vector"],
            "language": ["tx_fn_m0_bench_string_conversion_0", "tx_fn_m0_bench_module_call_0",
                         "_ZN12tx_generated16tx_int_to_stringB5cxx11Ex",
                         "_ZN12tx_generated18tx_float_to_stringB5cxx11Ed"],
            "diverse": ["tx_fn_m0_bench_parse_paths_0", "txrt_parse_int_scalar"],
            "borrowing": ["tx_fn_m0_bench_map_0", "txrt_map_read_i64_i64"],
        }
        for program, symbols in selected.items():
            for symbol in symbols:
                text = assembly(out / folder / (program + ".exe"), symbol)
                parts.append(text)
                calls[label][symbol] = {"calls": re.findall(r"call\s+[^\n]*<([^>]+)>", text),
                                        "normalized_sha256": signature(text)}
                if label == "new" and symbol == unhex_symbol:
                    calls[label][symbol]["vector_validation"] = bool(
                        re.search(r"\bpcmpeqb\b", text) and re.search(r"\bpsubusb\b", text))
        (archive / (label + "_hotpaths.asm")).write_text("\n".join(parts), encoding="utf-8")
    save("machine_calls.json", calls)


def bridges_and_sources():
    members = run("ar", "t", root / "tx/libtxstdlib.a")[0].splitlines()
    bridges = {}
    for name in ("httpx_bridge.o", "websocket_bridge.o", "requests_bridge.o"):
        assert members.count(name) == 1, name
        with tempfile.TemporaryDirectory(prefix="bridge_", dir=out) as temporary:
            subprocess.run(["ar", "x", str(root / "tx/libtxstdlib.a"), name], cwd=temporary, check=True)
            data = (Path(temporary) / name).read_bytes()
        assert data == (root / "build" / name).read_bytes(), name
        bridges[name] = hashlib.sha256(data).hexdigest()
    save("bridges.json", bridges)
    paths = [root / "src/stdlib/bytes.cpp", root / "scripts/check_performance_15_16.py",
             *archive.glob("*.py"), *archive.glob("*.cpp"), *archive.glob("*.tx"),
             *root.glob("tests/performance_15_16/*"),
             *root.glob("benchmarks/language_features/*.tx*"),
             root / "benchmarks/diverse_performance.tx", root / "benchmarks/call_borrowing.tx"]
    save("source_manifest.json", {str(path.relative_to(root)): digest(path) for path in paths})


def legacy_map():
    previous = json.loads((root / "benchmarks/performance_optimization_02_2026-09-29/call_borrowing_7_rounds.json").read_text(encoding="utf-8"))
    programs = {}
    for label, name in (("before_02", "call_borrowing_20260929.exe"), ("after_02", "call_borrowing_02.exe")):
        source = root / "tx_build" / name
        assert digest(source) == previous["source_and_program_sha256"]["old" if label == "before_02" else "new"]
        destination = out / "map_history" / label
        destination.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination / name)
        for dll in (root / "tx_build/performance_baseline_2026-09-29/legacy_bundle").glob("*.dll"):
            shutil.copy2(dll, destination / dll.name)
        programs[label] = destination / name
    programs["current"] = out / "candidate/borrowing.exe"
    result = sample(programs)
    assert len({json.dumps(value, sort_keys=True) for value in result["checksums"].values()}) == 1
    result["dll_pairing_limit"] = "Historical DLLs are the preserved stage 01 bundle; initial execution loader paths remain unproven."
    for label, program in programs.items():
        result[label + "_map_signature"] = signature(assembly(program, "txrt_map_read_i64_i64"))
    save("map_history.json", result)


if __name__ == "__main__":
    inspect_code()
    bridges_and_sources()
    if "--structure-only" not in sys.argv:
        legacy_map()
    print("PASS recorded optimized calls, bridge identity and historical map observation")
