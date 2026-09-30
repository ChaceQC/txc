"""静态执行体系提交后的全量串行复测，保留历史归档和原始样本。"""

import hashlib
import importlib.util
import os
from pathlib import Path
import shutil
import sys

root = Path(__file__).resolve().parents[2]
archive = Path(__file__).resolve().parent
previous = root / "benchmarks/performance_retest_2026-09-30_committed"
stdlib_archive = root / "benchmarks/stdlib_retest_2026-09-30_static_execution"
stdlib_work = root / "tx_build/stdlib_retest_2026_09_30_static_execution"


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def include_link_inputs(snapshot):
    # 默认 TX 编译已使用 ThinLTO；普通静态库指纹不足以证明实际链接输入稳定。
    def collect():
        values = snapshot()
        paths = [root / "tx/libtxstdlib_lto.a", root / "tx/link/ld.lld.exe",
                 root / "tx/link/ld.exe", root / "tx/package.compat"]
        paths.extend((root / "tx").glob("*.bc"))
        paths.extend((root / "tx/link").glob("*.o"))
        for path in paths:
            values[str(path.relative_to(root))] = hashlib.sha256(path.read_bytes()).hexdigest()
        return values
    return collect


def prepare_stdlib():
    stdlib_archive.mkdir(exist_ok=True)
    original = root / "benchmarks/stdlib_retest_2026-09-30_committed"
    for path in original.iterdir():
        if path.suffix in (".py", ".tx", ".cpp", ".hpp", ".java"):
            if path.name == "summarize.py":
                continue
            target = stdlib_archive / path.name
            if path.name == "prepare.py":
                text = path.read_text(encoding="utf-8").replace(
                    'work = root / "tx_build/stdlib_full_2026_09_30"',
                    'work = root / "tx_build/stdlib_retest_2026_09_30_static_execution"')
                target.write_text(text, encoding="utf-8")
            else:
                shutil.copy2(path, target)
    # 复用固定版本的公开依赖包，秘密夹具和历史样本均不复制。
    dependencies = root / "tx_build/stdlib_full_2026_09_30/dependencies"
    if dependencies.exists() and not (stdlib_work / "dependencies").exists():
        shutil.copytree(dependencies, stdlib_work / "dependencies")
    prepare = load("prepare", stdlib_archive / "prepare.py")
    print("START prepare new stdlib references", flush=True)
    prepare.main()


def main():
    if (archive / "manifest.json").exists() or (stdlib_archive / "manifest.json").exists():
        raise RuntimeError("拒绝覆盖已有正式采样；重跑须使用新归档目录")
    if (root / "build").exists():
        raise RuntimeError("请先完成 scripts/build.ps1 的正式封包及 build/ 清理")
    # 只允许新基准归档未提交，编译器和标准库必须来自已提交的工作树。
    import subprocess
    if subprocess.check_output(["git", "diff", "HEAD", "--", "src", "tx/stdlib",
                                "CMakeLists.txt", "scripts/build.ps1", "scripts/build_lto.py"],
                               cwd=root, encoding="utf-8"):
        raise RuntimeError("编译器或构建输入存在未提交改动")
    os.environ["PYTHONUTF8"] = "1"
    os.environ["PATH"] = str(root / "tx") + os.pathsep + os.environ["PATH"]
    prepare_stdlib()
    runner = load("static_execution_full_runner",
                  root / "benchmarks/performance_retest_2026-09-30_after_completion/run.py")
    runner.archive = archive
    runner.previous = previous
    runner.work = root / "tx_build/performance_retest_2026_09_30_static_execution"
    runner.legacy.archive = archive
    runner.legacy.work = runner.work
    runner.snapshot = include_link_inputs(runner.snapshot)
    runner.main()
    load("services", stdlib_archive / "services.py")
    stdlib = load("static_execution_stdlib_runner", stdlib_archive / "run.py")
    stdlib.snapshot = include_link_inputs(stdlib.snapshot)
    sys.argv = ["stdlib"]
    stdlib.main()


if __name__ == "__main__":
    main()
