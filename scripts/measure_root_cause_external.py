"""两道外部题的原始计时解、正式答案与留存旧程序对照，不修改题目源码。"""

from datetime import datetime, timezone
import json
import os
from pathlib import Path
import statistics
import subprocess

from check_static_execution import run
from measure_root_cause_repair import digest


root = Path(__file__).resolve().parents[1]
output = root / "tx_build/root_cause_repair_checks"
archive = root / "benchmarks/root_cause_repair_2026-09-30"
problems = {
    "mini": (Path("E:/Project/problems/mini-filesystem"),
             ("fanout-2000", "deep-pwd-2000", "linklong-2000", "moves-2000", "random-2000-1")),
    "stage": (Path("E:/Project/problems/not-yet-on-stage"),
              ("all-free-max", "alternating-tight-max", "forced-decreasing-max", "forced-increasing-max",
               "shuffled-tight-max-1")),
}


def measure(executable, data, expected):
    environment = os.environ.copy()
    environment["PATH"] = str(root / "tx") + os.pathsep + environment["PATH"]
    result = subprocess.run([str(executable)], cwd=output, env=environment, input=data,
                            capture_output=True, timeout=60)
    assert result.returncode == 0, result.stderr.decode("utf-8", "replace")
    assert result.stdout.decode("utf-8").split() == expected
    return int(result.stderr.decode("utf-8").strip()) / 1000


def main():
    output.mkdir(parents=True, exist_ok=True)
    result = {"utc": datetime.now(timezone.utc).isoformat(), "warmups": 1, "rounds": 3,
              "compiler_sha256": digest(root / "tx/txc.exe"), "problems": {}}
    for name, (project, cases) in problems.items():
        source = project / "src/solution_benchmark.tx"
        sources = {str(path): digest(path) for pattern in ("*.tx", "*.txh")
                   for path in (project / "src").glob(pattern)}
        before = root / "tx_build/language_comparison" / name / "tx.exe"
        after = output / (name + "_external.exe")
        run([root / "tx/txc.exe", source, "-o", after])
        entries = {}
        for case in cases:
            input_path = project / "testdata" / (case + ".in")
            expected_path = input_path.with_suffix(".out")
            data = input_path.read_bytes()
            expected = expected_path.read_text(encoding="utf-8").split()
            samples = {"before": [], "after": []}
            for round_ in range(4):
                order = (("before", before), ("after", after))
                if round_ % 2:
                    order = tuple(reversed(order))
                for label, executable in order:
                    elapsed = measure(executable, data, expected)
                    if round_:
                        samples[label].append(elapsed)
            entries[case] = {"input_sha256": digest(input_path), "answer_sha256": digest(expected_path),
                             "samples_ms": samples,
                             "medians_ms": {label: statistics.median(times) for label, times in samples.items()}}
            print(name, case, entries[case]["medians_ms"], flush=True)
        assert all(digest(Path(path)) == value for path, value in sources.items())
        result["problems"][name] = {"sources_sha256": sources, "before": str(before), "after": str(after),
                                    "before_sha256": digest(before), "after_sha256": digest(after), "cases": entries}
    (archive / "external_results.json").write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n",
                                                  encoding="utf-8")


if __name__ == "__main__":
    main()
