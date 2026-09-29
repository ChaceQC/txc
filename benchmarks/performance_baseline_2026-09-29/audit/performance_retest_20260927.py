"""Repeat the repository's benchmark programs and keep their raw samples."""

import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import statistics
import subprocess
import sys


ROOT = Path(__file__).resolve().parent.parent
AUDIT = ROOT / "tx_build/perf_audit_20260927"
OUTPUT = ROOT / "tx_build/performance_retest_20260927_committed.json"
JACKSON = Path.home() / ".m2/repository/com/fasterxml/jackson/core"
JAVA_CP = ";".join(str(path) for path in (
    AUDIT,
    JACKSON / "jackson-databind/2.18.4/jackson-databind-2.18.4.jar",
    JACKSON / "jackson-core/2.18.4.1/jackson-core-2.18.4.1.jar",
    JACKSON / "jackson-annotations/2.18.4/jackson-annotations-2.18.4.jar",
))


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, timeout=150):
    environment = os.environ.copy()
    environment["TX_PERF_AUDIT_VALUE"] = "sample"
    completed = subprocess.run(command, cwd=ROOT, capture_output=True, timeout=timeout,
                               env=environment)
    if completed.returncode:
        error = completed.stderr.decode("utf-8", errors="replace")[-1000:]
        raise RuntimeError(f"{command}: exit {completed.returncode}: {error}")
    return completed.stdout.decode("utf-8", errors="replace")


def triples(command, unit="us", timeout=150):
    lines = run(command, timeout).splitlines()
    if len(lines) % 3:
        raise ValueError(f"Bad triple output from {command}: {len(lines)} lines")
    result = {}
    for index in range(0, len(lines), 3):
        name = lines[index].strip()
        if name in result:
            raise ValueError(f"Duplicate benchmark: {name}")
        elapsed = float(lines[index + 1])
        checksum = lines[index + 2].strip()
        result[name] = {"ms": elapsed / 1000 if unit == "us" else elapsed,
                        "checksum": checksum}
    return result


def checksum_equal(left, right):
    if left == right:
        return True
    if left.lstrip("-").isdigit() and right.lstrip("-").isdigit():
        return int(left) == int(right)
    return math.isclose(float(left), float(right), rel_tol=1e-12, abs_tol=1e-6)


def suite(programs, rounds=3, warmups=1):
    samples = {language: {} for language in programs}
    checksums = {language: {} for language in programs}
    labels = list(programs)
    for _ in range(warmups):
        for label in labels:
            command, unit = programs[label]
            triples(command, unit)
    for index in range(rounds):
        order = labels if index % 2 == 0 else list(reversed(labels))
        for label in order:
            command, unit = programs[label]
            result = triples(command, unit)
            if checksums[label] and set(result) != set(checksums[label]):
                raise ValueError(f"Benchmark names changed in {label}")
            for name, measurement in result.items():
                old_checksum = checksums[label].setdefault(name, measurement["checksum"])
                if not checksum_equal(measurement["checksum"], old_checksum):
                    raise ValueError(f"Unstable checksum: {label} {name}")
                samples[label].setdefault(name, []).append(measurement["ms"])
        print(f"round {index + 1}/{rounds}: {', '.join(labels)}", flush=True)
    differences = {}
    for name in set.intersection(*(set(values) for values in checksums.values())):
        reference = checksums[labels[0]][name]
        if any(not checksum_equal(checksums[label][name], reference)
               for label in labels[1:]):
            differences[name] = {label: checksums[label][name] for label in labels}
    medians = {label: {name: statistics.median(values)
                       for name, values in group.items()}
               for label, group in samples.items()}
    return {"rounds": rounds, "warmups": warmups, "medians_ms": medians,
            "samples_ms": samples, "checksums": checksums,
            "cross_language_checksum_differences": differences}


def java(class_name):
    return ["java", "-cp", JAVA_CP, class_name]


def language_pair():
    return suite({
        "TX": ([str(ROOT / "tx_build/library_compare_tx.exe")], "ms"),
        "C++": ([str(ROOT / "tx_build/library_compare_cmake/library_compare_cpp.exe")], "ms"),
    }, rounds=7)


def audit_suites():
    audit = str(AUDIT)
    python = sys.executable
    groups = {
        "compute": {
            "TX": ([audit + "/compute_retest.exe"], "us"),
            "C++": ([audit + "/compute_cpp.exe"], "us"),
            "Python": ([python, "-B", audit + "/compute.py"], "us"),
            "Java": (java("ComputeBench"), "us"),
        },
        "features": {
            "TX": ([audit + "/features_retest.exe"], "us"),
            "C++": ([audit + "/features_cpp_retest.exe"], "us"),
            "Python": ([python, "-B", audit + "/features.py"], "us"),
            "Java": (java("FeatureBench"), "us"),
        },
        "system": {
            "TX": ([audit + "/system_retest.exe"], "us"),
            "C++": ([audit + "/system_cpp_retest.exe"], "us"),
            "Python": ([python, "-B", audit + "/system.py"], "us"),
            "Java": (java("SystemBench"), "us"),
        },
        "legacy_other": {
            "Python": ([python, "-B", audit + "/legacy.py"], "ms"),
            "Java": (java("LegacyBench"), "ms"),
        },
    }
    return {name: suite(programs) for name, programs in groups.items()}


def network_suites():
    # Each server is a child of this runner and is shut down after measurement.
    cases = (
        ("http", "http_server.py", {
            "TX": ([str(AUDIT / "http_retest.exe")], "us"),
            "C++": ([str(AUDIT / "http_cpp_retest.exe")], "us"),
            "Python": ([sys.executable, "-B", str(AUDIT / "http_client.py"),
                        "--requests-only"], "us"),
            "Java": (java("HttpBench"), "us"),
        }),
        ("websocket", "ws_server.py", {
            "TX": ([str(AUDIT / "ws_retest.exe")], "us"),
            "C++": ([str(AUDIT / "ws_cpp_retest.exe")], "us"),
            "Python": ([sys.executable, "-B", str(AUDIT / "ws_client.py")], "us"),
            "Java": (java("WsBench"), "us"),
        }),
    )
    results = {}
    for name, server_file, programs in cases:
        server = subprocess.Popen([sys.executable, "-B", str(AUDIT / server_file)],
                                  cwd=ROOT, stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE, text=True, encoding="utf-8")
        try:
            if server.stdout.readline().strip() != "READY":
                raise RuntimeError(f"{server_file} did not start")
            results[name] = suite(programs, rounds=3, warmups=1)
        finally:
            server.terminate()
            try:
                server.wait(timeout=5)
            except subprocess.TimeoutExpired:
                server.kill()
                server.wait(timeout=5)
            server.stdout.close()
            server.stderr.close()
    return results


def external_suites():
    comparison = ROOT / "tx_build/language_comparison/compare_languages.py"
    output = run([sys.executable, "-B", str(comparison)], timeout=1800)
    for line in output.splitlines():
        if not line.startswith("FINAL_REPORT "):
            print(line, flush=True)
    report = next(line[13:] for line in output.splitlines()
                  if line.startswith("FINAL_REPORT "))
    return json.loads(report)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("suite", choices=("library", "audit", "network", "external"))
    args = parser.parse_args()
    before = {name: digest(ROOT / name) for name in
              ("tx/txc.exe", "tx/libtxstdlib.a")}
    if args.suite == "library":
        result = language_pair()
    elif args.suite == "audit":
        result = audit_suites()
    elif args.suite == "network":
        result = network_suites()
    else:
        result = external_suites()
    after = {name: digest(ROOT / name) for name in before}
    if before != after:
        raise RuntimeError("Compiler or standard library changed while measuring")
    data = json.loads(OUTPUT.read_text(encoding="utf-8")) if OUTPUT.exists() else {}
    data[args.suite] = {"measured_at_utc": datetime.now(timezone.utc).isoformat(),
                        "toolchain_sha256": after, "results": result}
    OUTPUT.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n",
                      encoding="utf-8")
    print(f"Saved {args.suite} to {OUTPUT}", flush=True)


if __name__ == "__main__":
    main()
