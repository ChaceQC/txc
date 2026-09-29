"""Sequential five-language comparison with fixed inputs and internal timers."""

from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import statistics
import subprocess
import sys


ROOT = Path(__file__).resolve().parent
COMPILATION = ROOT.parents[1]
PROBLEMS = Path(r"E:\Project\problems")
TASKS = {
    "mini": ("mini-filesystem", [
        "fanout-2000", "deep-pwd-2000", "moves-2000", "random-2000-1", "linklong-2000",
    ]),
    "stage": ("not-yet-on-stage", [
        "all-free-max", "forced-increasing-max", "forced-decreasing-max",
        "shuffled-tight-max-1", "alternating-tight-max",
    ]),
}
LANGUAGES = ["C++", "JavaScript", "Java", "TX", "Python"]


def commands(key, project):
    source = PROBLEMS / project / "src"
    binary = ROOT / key
    return {
        "C++": [str(binary / "cpp.exe")],
        "JavaScript": ["node", str(source / "solution_benchmark_js.js")],
        "Java": ["java", "-cp", str(binary), "solution_benchmark_java"],
        "TX": [str(binary / "tx.exe")],
        "Python": [sys.executable, "-B", str(source / "solution_benchmark.py")],
    }


def checksum(path):
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def snapshot():
    files = [COMPILATION / "tx/txc.exe", COMPILATION / "tx/libtxstdlib.a"]
    for key, (project, cases) in TASKS.items():
        source = PROBLEMS / project / "src"
        files.extend(source.glob("solution_benchmark.*"))
        files.extend(source.glob("solution_benchmark_*.*"))
        files.extend(source.glob("mini_fs.*"))
        files.extend([source / "solution.cpp", source / "solution.py", source / "solution.tx"])
        files.extend((ROOT / key).glob("*.exe"))
        files.extend((ROOT / key).glob("*.class"))
        for case in cases:
            files.extend(PROBLEMS / project / "testdata" / (case + extension)
                         for extension in (".in", ".out"))
    return {str(path): checksum(path) for path in sorted(set(files))}


def execute(command, data, expected, key):
    result = subprocess.run(command, input=data, capture_output=True, timeout=30)
    normalize = bytes.splitlines if key == "mini" else bytes.split
    if result.returncode or normalize(result.stdout) != normalize(expected):
        raise RuntimeError((command, result.returncode, result.stderr.decode("utf-8")))
    return result


def main():
    before = snapshot()
    report = {
        "measured_at_utc": datetime.now(timezone.utc).isoformat(),
        "warmup_processes": 2,
        "measured_processes": 17,
        "timing": "internal microseconds; fresh process per invocation; sequential rotated order",
        "cpp_flags": "g++ 13.1.0 -std=c++23 -O3",
        "versions": {"node": "24.13.0", "java": "23.0.2", "python": sys.version.split()[0]},
        "tasks": {},
    }
    stage_data = PROBLEMS / "not-yet-on-stage/testdata"
    for binary in ("solution.exe", "tx.exe"):
        for path in sorted(stage_data.glob("*.in")):
            execute([str(ROOT / "stage" / binary)], path.read_bytes(),
                    path.with_suffix(".out").read_bytes(), "stage")
        print(f"not-yet-on-stage {binary}: 45/45 answers matched", flush=True)
    for key, (project, cases) in TASKS.items():
        task = {}
        programs = commands(key, project)
        for case_index, case in enumerate(cases):
            data_root = PROBLEMS / project / "testdata"
            data = (data_root / (case + ".in")).read_bytes()
            expected = (data_root / (case + ".out")).read_bytes()
            samples = {language: [] for language in LANGUAGES}
            for iteration in range(19):
                shift = (iteration + case_index) % len(LANGUAGES)
                for language in LANGUAGES[shift:] + LANGUAGES[:shift]:
                    result = execute(programs[language], data, expected, key)
                    elapsed = int(result.stderr.strip())
                    if iteration >= 2:
                        samples[language].append(elapsed)
            medians = {language: statistics.median(values) / 1000
                       for language, values in samples.items()}
            task[case] = {"samples_us": samples, "median_ms": medians}
            print(project, case, json.dumps(medians), flush=True)
        report["tasks"][project] = task
    after = snapshot()
    assert after == before, "Sources, toolchain, executables or inputs changed during measurement"
    report["stable_hashes"] = after
    print("FINAL_REPORT " + json.dumps(report, ensure_ascii=False), flush=True)


if __name__ == "__main__":
    main()
