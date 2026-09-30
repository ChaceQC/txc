"""原始负载与留存旧二进制交替采样；只报告根因修复涉及的条目。"""

from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import statistics

from check_static_execution import run


root = Path(__file__).resolve().parents[1]
output = root / "tx_build/root_cause_repair_checks"
archive = root / "benchmarks/root_cause_repair_2026-09-30"
previous = root / "tx_build/performance_retest_2026_09_30_static_execution/equivalence"
cases = {
    "features": (root / "benchmarks/performance_baseline_2026-09-29/audit/features.tx",
                 previous / "features_tx.exe", ("heap_push_pop", "iterator_snapshot")),
    "diverse": (root / "benchmarks/diverse_performance.tx", previous / "diverse_tx.exe",
                ("format_literal", "encoding_literal", "encoding_dynamic", "serde_short_text")),
    "concurrency": (root / "benchmarks/stdlib_retest_2026-09-30_static_execution/concurrency.tx",
                    root / "tx_build/stdlib_retest_2026_09_30_static_execution/concurrency.exe",
                    ("mutex_uncontended",)),
}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def measure(executable):
    lines = run([executable]).splitlines()
    if all(len(line.split()) == 3 for line in lines):
        lines = [part for line in lines for part in line.split()]
    assert len(lines) % 3 == 0
    return {lines[index]: {"ms": int(lines[index + 1]) / 1000,
                           "checksum": int(lines[index + 2])}
            for index in range(0, len(lines), 3)}


def main():
    output.mkdir(parents=True, exist_ok=True)
    archive.mkdir(parents=True, exist_ok=True)
    result = {"utc": datetime.now(timezone.utc).isoformat(), "warmups": 1, "rounds": 3,
              "compiler_sha256": digest(root / "tx/txc.exe"), "groups": {}}
    for name, (source, before, selected) in cases.items():
        after = output / (name + "_after.exe")
        run([root / "tx/txc.exe", source, "-o", after])
        warm_before = measure(before)
        warm_after = measure(after)
        expected = {key: value["checksum"] for key, value in warm_before.items()}
        assert expected == {key: value["checksum"] for key, value in warm_after.items()}
        samples = {"before": [], "after": []}
        for round_ in range(3):
            order = (("before", before), ("after", after))
            if round_ % 2:
                order = tuple(reversed(order))
            for label, executable in order:
                measured = measure(executable)
                assert expected == {key: value["checksum"] for key, value in measured.items()}
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
    (archive / "results.json").write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n",
                                         encoding="utf-8")


if __name__ == "__main__":
    main()
