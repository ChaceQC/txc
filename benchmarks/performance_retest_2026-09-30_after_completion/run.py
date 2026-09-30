"""提交后的全量串行复测；复用已有套件，不覆盖历史归档。"""

from datetime import datetime, timezone
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import time

import psutil

root = Path(__file__).resolve().parents[2]
archive = Path(__file__).resolve().parent
work = root / "tx_build/performance_retest_2026_09_30_after_completion"
previous = root / "benchmarks/performance_retest_2026-09-30"
os.environ["PYTHONUTF8"] = "1"
os.environ["PATH"] = str(root / "tx") + os.pathsep + os.environ["PATH"]


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


legacy = load("full_retest", previous / "run.py")
legacy.archive = archive
legacy.work = work


def snapshot():
    files = legacy.snapshot()
    paths = [root / "CMakeLists.txt", root / "AGENTS.md", root / "tx/clang.exe",
             root / "tx_build/performance_retest_20260927.py",
             root / "tx_build/language_comparison/compare_languages.py"]
    paths.extend((root / "tx").glob("*.o"))
    paths.extend((root / "tx/link").glob("*.a"))
    audit = root / "tx_build/perf_audit_20260927"
    for pattern in ("*.tx", "*.cpp", "*.py", "*.java", "*.class"):
        paths.extend(audit.glob(pattern))
    paths.extend(audit / name for name in
                 ("compute_cpp.exe", "features_cpp_retest.exe", "system_cpp_retest.exe",
                  "http_cpp_retest.exe", "ws_cpp_retest.exe", "child.exe"))
    for key, project in (("mini", "mini-filesystem"), ("stage", "not-yet-on-stage")):
        project_dir = Path("E:/Project/problems") / project
        for path in project_dir.joinpath("src").iterdir():
            if path.suffix in (".tx", ".txh", ".cpp", ".hpp", ".py", ".java", ".js"):
                paths.append(path)
        paths.extend(project_dir.joinpath("testdata").glob("*.in"))
        paths.extend(project_dir.joinpath("testdata").glob("*.out"))
        binary = root / "tx_build/language_comparison" / key
        paths.append(binary / "cpp.exe")
        paths.extend(binary.glob("*.class"))
    for path in sorted(set(paths)):
        files[os.path.relpath(path, root).replace("\\", "/")] = hashlib.sha256(path.read_bytes()).hexdigest()
    return files


def versions():
    values = {}
    for name, command in {
        "python": [sys.executable, "--version"],
        "gcc": ["g++", "--version"],
        "clang": [root / "tx/clang.exe", "--version"],
        "java": ["java", "-version"],
        "node": ["node", "--version"],
    }.items():
        result = subprocess.run([str(arg) for arg in command], cwd=root,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                encoding="utf-8", errors="strict", timeout=30, check=True)
        values[name] = result.stdout.strip()
    return values


def diverse():
    module = load("diverse_retest", root / "scripts/run_diverse_performance.py")
    module.output = archive / "diverse.json"
    module.main()


def equivalent():
    module = load("equivalent_retest", root / "scripts/run_performance_equivalence.py")
    module.work = work / "equivalence"
    sys.argv = ["equivalent", "--rounds", "5"]
    module.main()
    shutil.copy2(module.work / "results_complete.json", archive / "equivalence.json")


def contracts():
    module = load("completion_contracts", root / "scripts/check_performance_completion.py")
    module.work = work / "contracts"
    module.work.mkdir(parents=True, exist_ok=True)
    # archive 仍定位原始基准源码，只有样本输出改到本轮目录。
    module.save = legacy.save
    module.contracts()


def main():
    if (archive / "manifest.json").exists():
        raise RuntimeError("拒绝覆盖本轮样本；再次复测请使用新的归档目录")
    if (root / "build").exists():
        raise RuntimeError("请先完成 scripts/build.ps1 的正式封包及 build/ 清理")
    work.mkdir(parents=True, exist_ok=True)
    before = snapshot()
    legacy.save("manifest.json", {
        "started_utc": datetime.now(timezone.utc).isoformat(),
        "head": legacy.run("git", "rev-parse", "HEAD").strip(),
        "initial_status": legacy.run("git", "status", "--short"),
        "sha256": before,
        "machine": {"platform": platform.platform(), "processor": platform.processor(),
                    "logical_cpus": os.cpu_count(), "physical_cpus": psutil.cpu_count(logical=False),
                    "memory_gib": psutil.virtual_memory().total / (1024 ** 3)},
        "versions": versions(),
        "previous_archive": str(previous.relative_to(root)),
        "reference_policy": "语言/库/综合/校准/新增契约重新构建；模块审计及外部题的 C++/Java 参考沿用并记录哈希",
    })
    timings = {}
    phases = [("language", legacy.language), ("audit", legacy.audit),
              ("special", legacy.special), ("diverse", diverse),
              ("equivalent", equivalent), ("contracts", contracts)]
    for name, action in phases:
        print(f"START {name}", flush=True)
        started = time.perf_counter()
        action()
        timings[name] = time.perf_counter() - started
        print(f"PASS {name}: {timings[name]:.1f} s", flush=True)
    after = snapshot()
    changed = sorted(name for name in set(before) | set(after) if before.get(name) != after.get(name))
    if changed:
        legacy.save("changed_inputs.json", changed)
        raise RuntimeError(f"测量期间源码、工具链、参考程序或输入改变：{changed}")
    legacy.save("completed.json", {
        "completed_utc": datetime.now(timezone.utc).isoformat(),
        "source_and_toolchain_unchanged": True,
        "reference_programs_and_external_inputs_unchanged": True,
        "phase_seconds": timings,
    })
    print("ALL PERFORMANCE SUITES COMPLETE", flush=True)


if __name__ == "__main__":
    main()
