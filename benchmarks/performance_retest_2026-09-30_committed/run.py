"""提交后完整复测，复用现有工作量，历史样本保持不变。"""
import importlib.util
import shutil
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[2]
archive = Path(__file__).resolve().parent
previous = root / "benchmarks/performance_retest_2026-09-30_full_rerun"
stdlib_archive = root / "benchmarks/stdlib_retest_2026-09-30_committed"


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def main():
    if (archive / "manifest.json").exists() or (stdlib_archive / "completed.json").exists():
        raise RuntimeError("拒绝覆盖已有正式采样")
    stdlib_archive.mkdir(exist_ok=True)
    original = root / "benchmarks/stdlib_full_2026-09-30"
    # 只复制基准源码；不复制历史结果或含秘密的运行夹具。
    for path in original.iterdir():
        if path.suffix in (".py", ".tx", ".cpp", ".hpp", ".java") and path.name != "summarize.py":
            shutil.copy2(path, stdlib_archive / path.name)
    prepare = load("prepare", stdlib_archive / "prepare.py")
    print("START prepare new stdlib references", flush=True)
    prepare.main()
    runner = load("committed_full_runner", root / "benchmarks/performance_retest_2026-09-30_after_completion/run.py")
    runner.archive = archive
    runner.previous = previous
    runner.work = root / "tx_build/performance_retest_2026_09_30_committed"
    runner.legacy.archive = archive
    runner.legacy.work = runner.work
    runner.main()
    load("services", stdlib_archive / "services.py")
    stdlib = load("committed_stdlib_runner", stdlib_archive / "run.py")
    sys.argv = ["stdlib"]
    stdlib.main()


if __name__ == "__main__":
    main()
