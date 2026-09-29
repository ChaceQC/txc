"""同会话交替测量 TX 基线/候选，并分离堆算法核心与 TX ABI 成本。"""
import hashlib
import json
import os
from pathlib import Path
import statistics
import subprocess
import sys

root = Path(__file__).resolve().parents[2]
archive = Path(__file__).resolve().parent
output = root / "tx_build/performance_09_11"
environment = os.environ.copy()
environment["PATH"] = str(root / "tx") + os.pathsep + environment["PATH"]
sys.path.insert(0, str(root / "scripts"))
from compare_performance_programs import read_program


def run(*arguments):
    result = subprocess.run([str(value) for value in arguments], cwd=root, env=environment,
                            capture_output=True, encoding="utf-8", errors="strict", timeout=120)
    assert result.returncode == 0, result.stdout + result.stderr


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    candidate = output / "candidate/paths.exe"
    candidate.parent.mkdir(parents=True, exist_ok=True)
    run(root / "tx/txc.exe", archive / "paths.tx", "-o", candidate)
    baseline = json.loads((archive / "baseline.json").read_text(encoding="utf-8"))
    old_header = output / "heap_baseline/stdlib/typed_heap.hpp"
    old_header.parent.mkdir(parents=True, exist_ok=True)
    original = subprocess.check_output(["git", "show", baseline["head"] +
                                       ":src/stdlib/typed_heap.hpp"], cwd=root)
    old_header.write_bytes(original)
    programs = {"old_tx": output / "baseline/paths.exe", "new_tx": candidate}
    for label, include in (("old_heap_core", old_header.parents[1]),
                           ("new_heap_core", root / "src")):
        target = output / (label + ".exe")
        run("g++", "-std=c++23", "-O3", "-finput-charset=UTF-8", "-fexec-charset=UTF-8",
            "-I" + str(include), "-Isrc", archive / "heap_core.cpp", root / "tx/libtxstdlib.a",
            "-Ltx/link", "-lwinhttp", "-lws2_32", "-ladvapi32", "-lbcrypt", "-lshell32",
            "-luser32", "-liconv", "-o", target)
        programs[label] = target
    # 编译器复制候选 DLL；原生核心在根 tx_build 子目录通过 PATH 取得同套 DLL。
    os.environ["PATH"] = environment["PATH"]
    reference = {label: read_program(path) for label, path in programs.items()}
    assert reference["old_tx"].keys() == reference["new_tx"].keys()
    for measurements in reference.values():
        for name, (_, checksum) in measurements.items():
            assert checksum == reference["old_tx"][name][1], (name, checksum)
    samples = {label: {name: [] for name in values} for label, values in reference.items()}
    orders = []
    for iteration in range(5):
        order = list(programs) if iteration % 2 == 0 else list(reversed(programs))
        orders.append(order)
        for label in order:
            measurements = read_program(programs[label])
            for name, (milliseconds, checksum) in measurements.items():
                assert checksum == reference[label][name][1], (label, name)
                samples[label][name].append(milliseconds)
    artifacts = [root / "tx/txc.exe", root / "tx/libtxstdlib.a", root / "tx/package.compat",
                 *programs.values(), archive / "paths.tx", archive / "heap_core.cpp", old_header]
    report = {"warmups": 1, "rounds": 5, "orders": orders, "samples_ms": samples,
              "medians_ms": {label: {name: statistics.median(values) for name, values in cases.items()}
                             for label, cases in samples.items()},
              "checksums": {name: checksum for name, (_, checksum) in reference["old_tx"].items()},
              "sha256": {str(path.relative_to(root)): digest(path) for path in artifacts}}
    (archive / "samples.json").write_text(json.dumps(report, ensure_ascii=False, indent=2)
                                          + "\n", encoding="utf-8")
    print(json.dumps(report["medians_ms"], ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
