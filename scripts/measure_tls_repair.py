"""隔离 TLS 实现选项，避免把跨轮机器状态解释成根因。"""

from datetime import datetime, timezone
import importlib.util
import json
from pathlib import Path
import shutil
import statistics
import sys

from measure_root_cause_remaining import root, output, archive, old_stdlib, stdlib_sources, execute, compile_program
from measure_root_cause_repair import digest


def main():
    everest = output / "network_everest.exe"
    if "--capture-everest" in sys.argv:
        shutil.copy2(output / "network_remaining.exe", everest)
        print("Captured Everest candidate")
        return
    source = stdlib_sources / "network.tx"
    candidate = compile_program("network_final", source)
    final = "--final" in sys.argv
    programs = {"before": old_stdlib / "network.exe", "candidate": candidate}
    if not final:
        programs["everest"] = everest
    spec = importlib.util.spec_from_file_location("tls_repair_servers", stdlib_sources / "services.py")
    services = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(services)
    samples = {label: [] for label in programs}
    expected = None
    with services.servers(old_stdlib / "fixtures") as environment:
        for round_ in range(6):
            order = list(programs)
            offset = round_ % len(order)
            order = order[offset:] + order[:offset]
            for label in order:
                values = execute(programs[label], "network", environment)
                checksums = {key: value["checksum"] for key, value in values.items()}
                expected = checksums if expected is None else expected
                assert expected == checksums
                if round_:
                    samples[label].append(values["tls_handshake"]["ms"])
    result = {"utc": datetime.now(timezone.utc).isoformat(), "rounds": 5, "warmups": 1,
              "compiler_sha256": digest(root / "tx/txc.exe"),
              "config_sha256": digest(root / "cmake/mbedtls_user_config.h"),
              "candidate_everest_enabled": "#define MBEDTLS_ECDH_VARIANT_EVEREST_ENABLED" in
                  (root / "cmake/mbedtls_user_config.h").read_text(encoding="utf-8"),
              "source_sha256": digest(source), "programs": {label: {"path": str(path), "sha256": digest(path)}
                                                             for label, path in programs.items()},
              "samples_ms": samples, "medians_ms": {label: statistics.median(times) for label, times in samples.items()},
              "checksums": expected}
    report = "tls_final.json" if final else "tls_selection.json"
    (archive / report).write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(result["medians_ms"])


if __name__ == "__main__":
    main()
