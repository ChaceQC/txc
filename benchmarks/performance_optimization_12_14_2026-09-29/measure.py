"""一轮预热、五轮正反交替；保存全部样本、图回收延迟和进程峰值内存。"""
import hashlib
import json
import os
from pathlib import Path
import statistics
import subprocess

root = Path(__file__).resolve().parents[2]
archive = Path(__file__).resolve().parent
out = root / "tx_build/performance_12_14"
environment = os.environ.copy()
environment["PATH"] = str(root / "tx") + os.pathsep + environment["PATH"]


def run(*arguments):
    result = subprocess.run([str(value) for value in arguments], cwd=root,
                            env=environment, capture_output=True, encoding="utf-8",
                            errors="strict", timeout=120)
    assert result.returncode == 0, (arguments, result.stdout, result.stderr)
    return result


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_program(command):
    result = run(*command)
    lines = result.stdout.splitlines()
    assert len(lines) % 3 == 0, result.stdout
    measurements = {lines[index]: (float(lines[index + 1]) / 1000, int(lines[index + 2]))
                    for index in range(0, len(lines), 3)}
    return measurements, json.loads(result.stderr) if result.stderr.strip() else {}


def main():
    candidate = out / "candidate/paths.exe"
    candidate.parent.mkdir(parents=True, exist_ok=True)
    run(root / "tx/txc.exe", archive / "paths.tx", "-o", candidate)
    baseline = json.loads((archive / "baseline.json").read_text(encoding="utf-8"))
    baseline["sha256"] = {name.replace("\\", "/"): value for name, value in baseline["sha256"].items()}
    assert digest(out / "baseline/libtxstdlib.a") == baseline["sha256"]["tx/libtxstdlib.a"]
    assert digest(archive / "paths.tx") == baseline["sha256"][(archive / "paths.tx").relative_to(root).as_posix()]
    programs = {"old_tx": [out / "baseline/paths.exe"], "new_tx": [candidate]}
    for label, library in (("old_graph", out / "baseline/libtxstdlib.a"),
                           ("new_graph", root / "tx/libtxstdlib.a")):
        executable = out / (label + ".exe")
        run("g++", "-std=c++23", "-O3", "-finput-charset=UTF-8", "-fexec-charset=UTF-8",
            "-Isrc", archive / "graph_core.cpp", library, "-Ltx/link", "-lwinhttp",
            "-lws2_32", "-ladvapi32", "-lbcrypt", "-lshell32", "-luser32", "-liconv",
            "-lpsapi", "-o", executable)
        programs[label] = [executable]
    programs["cpp_graph"] = [out / "new_graph.exe", "reference"]
    vector = out / "vector_reference.exe"
    run("g++", "-std=c++23", "-O3", "-finput-charset=UTF-8", "-fexec-charset=UTF-8",
        archive / "vector_reference.cpp", "-o", vector)
    programs["cpp_vector"] = [vector]
    reference = {label: read_program(command)[0] for label, command in programs.items()}
    expected = {}
    for results in reference.values():
        for name, (_, checksum) in results.items():
            assert expected.setdefault(name, checksum) == checksum, (name, checksum)
    samples = {label: {name: [] for name in cases} for label, cases in reference.items()}
    resources = {label: [] for label in programs}
    orders = []
    for iteration in range(5):
        order = list(programs) if iteration % 2 == 0 else list(reversed(programs))
        orders.append(order)
        for label in order:
            measurements, memory = read_program(programs[label])
            assert measurements.keys() == reference[label].keys()
            for name, (elapsed, checksum) in measurements.items():
                assert checksum == expected[name], (label, name, checksum)
                samples[label][name].append(elapsed)
            resources[label].append(memory)
    artifacts = [root / "tx/txc.exe", root / "tx/libtxstdlib.a", root / "tx/package.compat",
                 archive / "paths.tx", archive / "graph_core.cpp", archive / "vector_reference.cpp",
                 *(command[0] for command in programs.values())]
    report = {"warmups": 1, "rounds": 5, "orders": orders, "samples_ms": samples,
              "medians_ms": {label: {name: statistics.median(values) for name, values in cases.items()}
                             for label, cases in samples.items()},
              "resources": resources, "checksums": expected,
              "sha256": {str(path.relative_to(root)): digest(path) for path in artifacts}}
    (archive / "samples.json").write_text(json.dumps(report, ensure_ascii=False, indent=2)
                                          + "\n", encoding="utf-8")
    print(json.dumps(report["medians_ms"], ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
