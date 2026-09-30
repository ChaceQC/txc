"""剩余根因项的串行配对采样，保存原始程序与输入哈希，不覆盖首批样本。"""

from contextlib import nullcontext
from datetime import datetime, timezone
import importlib.util
import json
import os
from pathlib import Path
import shutil
import statistics
import subprocess
import tempfile

from check_static_execution import run
from measure_root_cause_repair import digest


root = Path(__file__).resolve().parents[1]
output = root / "tx_build/root_cause_repair_checks"
archive = root / "benchmarks/root_cause_repair_2026-09-30"
old_stdlib = root / "tx_build/stdlib_retest_2026_09_30_static_execution"
old_performance = root / "tx_build/performance_retest_2026_09_30_static_execution"
stdlib_sources = root / "benchmarks/stdlib_retest_2026-09-30_static_execution"
groups = {
    "language": (root / "benchmarks/language_features/compare.tx", root / "tx_build/language_features_tx.exe",
                 ("deinit", "deep_copy", "copy_cycle", "cycle_gc", "string_conversion")),
    "dictionary": (root / "benchmarks/diverse_performance.tx", old_performance / "equivalence/diverse_tx.exe",
                   ("dictionary_int_hit", "dictionary_text_hit", "dictionary_mostly_miss")),
    "diagnostics": (stdlib_sources / "diagnostics.tx", old_stdlib / "diagnostics.exe",
                    ("test_parameterized", "test_property")),
    "database": (stdlib_sources / "database.tx", old_stdlib / "database.exe", ("sqlite_pool", "sqlite_async")),
    "network": (stdlib_sources / "network.tx", old_stdlib / "network.exe", ("tls_handshake",)),
    "random": (root / "benchmarks/library_compare/random_profile.tx", old_performance / "random.exe",
               ("random_long",)),
    "graph": (root / "benchmarks/performance_completion_2026-09-30/graph_contract.cpp",
              old_performance / "contracts/graph_contract.exe", ("graph_copy", "graph_gc")),
}


def execute(executable, name, extra=None):
    environment = os.environ.copy()
    environment["PATH"] = str(root / "tx") + os.pathsep + environment["PATH"]
    environment["BENCH_GROUP"] = "sqlite"
    environment.update(extra or {})
    with tempfile.TemporaryDirectory(prefix="root_cause_sample_", dir=output) as temporary:
        directory = Path(temporary)
        if name == "network":
            for file in ("message.bin", "ca.der"):
                shutil.copy2(old_stdlib / "fixtures" / file, directory / file)
        result = subprocess.run([str(executable)], cwd=directory, env=environment, capture_output=True,
                                encoding="utf-8", errors="strict", timeout=60)
    assert result.returncode == 0, result.stdout + result.stderr
    lines = result.stdout.splitlines()
    if name == "random":
        assert len(lines) == 2
        return {"random_long": {"ms": float(lines[0]), "checksum": int(lines[1])}}
    if all(len(line.split()) == 3 for line in lines):
        lines = [part for line in lines for part in line.split()]
    assert len(lines) % 3 == 0
    return {lines[index]: {"ms": float(lines[index + 1]) / 1000, "checksum": int(lines[index + 2])}
            for index in range(0, len(lines), 3)}


def compile_program(name, source):
    executable = output / (name + "_remaining.exe")
    if source.suffix == ".tx":
        run([root / "tx/txc.exe", source, "-o", executable])
    else:
        run(["g++", "-std=c++23", "-O3", "-finput-charset=UTF-8", "-fexec-charset=UTF-8", "-Isrc",
             source, root / "tx/libtxstdlib.a", "-Ltx/link", "-lwinhttp", "-lws2_32", "-ladvapi32",
             "-lbcrypt", "-lshell32", "-luser32", "-liconv", "-lpsapi", "-o", executable])
    return executable


def main():
    output.mkdir(parents=True, exist_ok=True)
    archive.mkdir(parents=True, exist_ok=True)
    spec = importlib.util.spec_from_file_location("root_cause_servers", stdlib_sources / "services.py")
    services = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(services)
    result = {"utc": datetime.now(timezone.utc).isoformat(), "warmups": 1, "rounds": 3,
              "compiler_sha256": digest(root / "tx/txc.exe"), "groups": {}}
    for name, (source, before, selected) in groups.items():
        after = compile_program(name, source)
        samples = {"before": [], "after": []}
        expected = None
        context = services.servers(old_stdlib / "fixtures") if name == "network" else nullcontext({})
        with context as extra:
            for round_ in range(4):
                order = (("before", before), ("after", after))
                if round_ % 2:
                    order = tuple(reversed(order))
                for label, executable in order:
                    measured = execute(executable, name, extra)
                    checksums = {key: value["checksum"] for key, value in measured.items()}
                    if expected is None:
                        expected = checksums
                    assert checksums == expected, (name, label, checksums, expected)
                    if round_:
                        samples[label].append(measured)
        entries = {}
        for key in selected:
            entries[key] = {"checksum": expected[key]}
            for label in samples:
                times = [sample[key]["ms"] for sample in samples[label]]
                entries[key][label + "_ms"] = times
                entries[key][label + "_median_ms"] = statistics.median(times)
            print(key, entries[key]["before_median_ms"], "->", entries[key]["after_median_ms"], flush=True)
        result["groups"][name] = {
            "source": str(source.relative_to(root)), "source_sha256": digest(source),
            "before": str(before.relative_to(root)), "before_sha256": digest(before),
            "after": str(after.relative_to(root)), "after_sha256": digest(after),
            "entries": entries, "samples": samples,
        }
        (archive / "remaining_results.json").write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n",
                                                       encoding="utf-8")


if __name__ == "__main__":
    main()
