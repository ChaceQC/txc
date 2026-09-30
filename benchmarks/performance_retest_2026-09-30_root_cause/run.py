"""根因修复工作区全量复测；复用原套件并保留源码与构建指纹。"""

from datetime import datetime, timezone
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

root = Path(__file__).resolve().parents[2]
archive = Path(__file__).resolve().parent
previous = root / "benchmarks/performance_retest_2026-09-30_static_execution"
stdlib_archive = archive / "stdlib"
stdlib_work = root / "tx_build/stdlib_retest_2026_09_30_root_cause"


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def save(name, value):
    (archive / name).write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n",
                               encoding="utf-8")


def source_snapshot():
    paths = [root / "CMakeLists.txt", root / "AGENTS.md"]
    for folder in ("src", "cmake", "tx/stdlib"):
        paths.extend(path for path in (root / folder).rglob("*") if path.is_file())
    paths.extend(root / "scripts" / name for name in ("build.ps1", "build_lto.py"))
    return {path.relative_to(root).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in sorted(set(paths))}


def build():
    if (archive / "build_completed.json").exists():
        raise RuntimeError("拒绝覆盖本轮构建记录")
    before = source_snapshot()
    save("build_manifest.json", {"started_utc": datetime.now(timezone.utc).isoformat(),
         "head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root,
                                          encoding="utf-8").strip(),
         "initial_status": subprocess.check_output(["git", "status", "--short"], cwd=root,
                                                    encoding="utf-8"),
         "source_sha256": before,
         "policy": "测试当前未提交工作区；HEAD 仅作为基准身份，完整源码指纹定义本轮版本"})
    # 发行目录含 clang，显式选择与 g++ 配套的 GCC，避免 CMake 自动混用工具链。
    llvm_bin = root.parent / ".llvm-dev/extracted/LLVM/bin"
    build_environment = {**os.environ, "CC": shutil.which("gcc"), "CXX": shutil.which("g++"),
                         "TX_LLVM_BIN": str(llvm_bin)}
    log_name = "build.log"
    for attempt in range(1, 100):
        if not (archive / log_name).exists():
            break
        log_name = f"build_retry_{attempt}.log"
    with (archive / log_name).open("w", encoding="utf-8") as output:
        result = subprocess.run(["pwsh", "-NoProfile", "-File", str(root / "scripts/build.ps1")],
                                cwd=root, env=build_environment, stdout=output, stderr=subprocess.STDOUT)
    if result.returncode:
        raise RuntimeError(f"正式构建失败，见 {log_name}；保留 build/ 排查")
    if source_snapshot() != before:
        raise RuntimeError("构建期间源码改变，不能作为同一版本采样")
    if (root / "build").exists():
        raise RuntimeError("构建目录尚未清理")
    save("build_completed.json", {"completed_utc": datetime.now(timezone.utc).isoformat(),
         "source_unchanged": True, "build_log": log_name,
         "package_sha256": {name: hashlib.sha256((root / "tx" / name).read_bytes()).hexdigest()
                            for name in ("txc.exe", "libtxstdlib.a", "libtxstdlib_lto.a", "package.compat")}})
    print("PASS official build and package", flush=True)


def include_inputs(snapshot):
    previous_runner = load("root_cause_previous_runner", previous / "run.py")
    linked = previous_runner.include_link_inputs(snapshot)

    def collect():
        values = linked()
        values.update(source_snapshot())
        for folder in (archive, root / "scripts"):
            for path in folder.rglob("*.py"):
                values[path.relative_to(root).as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()
        return values
    return collect


def prepare_stdlib():
    original = root / "benchmarks/stdlib_retest_2026-09-30_static_execution"
    prepare = load("prepare", original / "prepare.py")
    prepare.work = stdlib_work
    dependencies = root / "tx_build/stdlib_full_2026_09_30/dependencies"
    if dependencies.exists() and not (stdlib_work / "dependencies").exists():
        shutil.copytree(dependencies, stdlib_work / "dependencies")
    print("START prepare standard library references", flush=True)
    prepare.main()
    return original


def run_stdlib(original):
    # 原采样器用 source 同时定位参考程序与防覆盖记录，必须一起指向新归档。
    stdlib_archive.mkdir(exist_ok=True)
    for path in original.iterdir():
        if path.suffix in (".tx", ".cpp", ".hpp", ".java") or path.name == "reference.py":
            shutil.copy2(path, stdlib_archive / path.name)
    load("services", original / "services.py")
    stdlib = load("root_cause_stdlib_runner", original / "run.py")
    stdlib.source = stdlib_archive
    stdlib.snapshot = include_inputs(stdlib.snapshot)
    sys.argv = ["stdlib"]
    stdlib.main()


def main():
    os.environ["PYTHONUTF8"] = "1"
    os.environ["PATH"] = str(root / "tx") + os.pathsep + os.environ["PATH"]
    if "--build-only" in sys.argv:
        build()
        return
    resume_stdlib = "--stdlib-only" in sys.argv
    if (not resume_stdlib and (archive / "manifest.json").exists()) or (stdlib_archive / "manifest.json").exists():
        raise RuntimeError("拒绝覆盖已有采样；重跑请使用新目录")
    built = json.loads((archive / "build_manifest.json").read_text(encoding="utf-8"))
    package = json.loads((archive / "build_completed.json").read_text(encoding="utf-8"))
    if built["source_sha256"] != source_snapshot():
        raise RuntimeError("当前源码与本轮构建记录不一致")
    for name, digest in package["package_sha256"].items():
        if hashlib.sha256((root / "tx" / name).read_bytes()).hexdigest() != digest:
            raise RuntimeError("当前发行包与本轮构建记录不一致")
    if resume_stdlib:
        completed = json.loads((archive / "completed.json").read_text(encoding="utf-8"))
        if not completed["source_and_toolchain_unchanged"]:
            raise RuntimeError("原有套件未通过输入稳定性检查")
        original = root / "benchmarks/stdlib_retest_2026-09-30_static_execution"
        prepare = load("prepare", original / "prepare.py")
        prepare.work = stdlib_work
        save("stdlib_resume.json", {"resumed_utc": datetime.now(timezone.utc).isoformat(),
             "reason": "原采样器 source 指向历史目录，在开始标准库采样前触发防覆盖检查；只续跑标准库",
             "core_completed": True, "source_and_package_match_build": True,
             "core_runner_snapshot": "run_core_snapshot.py"})
        run_stdlib(original)
        return
    stdlib_archive.mkdir(exist_ok=True)
    original = prepare_stdlib()
    runner = load("root_cause_full_runner",
                  root / "benchmarks/performance_retest_2026-09-30_after_completion/run.py")
    runner.archive = archive
    runner.previous = previous
    runner.work = root / "tx_build/performance_retest_2026_09_30_root_cause"
    runner.legacy.archive = archive
    runner.legacy.work = runner.work
    runner.snapshot = include_inputs(runner.snapshot)
    runner.main()
    run_stdlib(original)
    print("PASS all full performance suites", flush=True)


if __name__ == "__main__":
    if "--build-only" in sys.argv or "--sample-child" in sys.argv:
        main()
    else:
        # 子进程串行执行，保留完整 UTF-8 日志而不混入性能程序的标准输出。
        log_name = "stdlib_run.log" if "--stdlib-only" in sys.argv else "run.log"
        with (archive / log_name).open("x", encoding="utf-8") as output:
            result = subprocess.run([sys.executable, "-X", "utf8", "-B", str(Path(__file__).resolve()),
                                     "--sample-child", *sys.argv[1:]], cwd=root,
                                    stdout=output, stderr=subprocess.STDOUT)
        if result.returncode:
            raise RuntimeError(f"性能采样失败，见 {log_name}；已有样本保留")
        print("PASS full performance run; see run.log", flush=True)
