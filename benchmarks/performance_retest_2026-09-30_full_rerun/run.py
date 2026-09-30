"""复用完整性能套件，将新一轮样本保存在独立归档中。"""

import importlib.util
from pathlib import Path

root = Path(__file__).resolve().parents[2]
archive = Path(__file__).resolve().parent
previous = root / "benchmarks/performance_retest_2026-09-30_after_completion"


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main():
    runner = load("previous_full_runner", previous / "run.py")
    runner.archive = archive
    runner.previous = previous
    runner.work = root / "tx_build/performance_retest_2026_09_30_full_rerun"
    runner.legacy.archive = archive
    runner.legacy.work = runner.work
    runner.main()


if __name__ == "__main__":
    main()
