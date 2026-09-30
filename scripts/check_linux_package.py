"""验证实际 Linux tar.gz，使用包内 LLVM/库，在中文空格路径运行最小安装测试。"""

from pathlib import Path
import hashlib
import os
import subprocess
import tarfile
import tempfile

from linux_bundle import require_bundled_dependencies


root = Path(__file__).resolve().parents[1]


def run(arguments, directory, environment):
    result = subprocess.run([str(item) for item in arguments], cwd=directory, env=environment,
        capture_output=True, encoding="utf-8", errors="replace", timeout=180)
    if result.returncode:
        raise RuntimeError(f"安装验证失败：{arguments}\n{result.stdout}\n{result.stderr}")
    return result.stdout


def main():
    archives = list((root / "dist").glob("*linux-x64.tar.gz"))
    if len(archives) != 1:
        raise RuntimeError("本轮必须恰有一个 Linux 发行包")
    archive = archives[0]
    expected = (root / "dist/SHA256SUMS.txt").read_text(encoding="utf-8").split()[0]
    with archive.open("rb") as stream:
        if hashlib.file_digest(stream, "sha256").hexdigest() != expected:
            raise RuntimeError("Linux 包 SHA-256 不匹配")
    with tempfile.TemporaryDirectory(prefix="txc 安装验证 ") as temporary:
        installed = Path(temporary)
        with tarfile.open(archive) as package:
            package.extractall(installed, filter="data")
        directory = installed / archive.name.removesuffix(".tar.gz")
        environment = os.environ.copy()
        # clang/ld.lld 必须来自包内；清除开发机的动态库搜索路径。
        environment["PATH"] = "/nonexistent"
        environment.pop("LD_LIBRARY_PATH", None)
        environment.pop("LD_PRELOAD", None)
        environment.pop("TX_LLVM_BIN", None)
        compiler = directory / "tx/txc"
        for executable in (compiler, directory / "tx/clang", directory / "tx/link/ld.lld"):
            require_bundled_dependencies(executable, directory / "tx/lib")
        run([compiler, "check", directory / "example.tx"], directory, environment)
        for name in ("llvm_numeric", "unicode"):
            for lto in (True, False):
                target = directory / (name + ("-lto" if lto else "-native"))
                run([compiler, directory / f"examples/{name}.tx", "-o", target,
                    *([] if lto else ["--no-lto"])], directory, environment)
                require_bundled_dependencies(target, directory / "tx_lib")
                output = run([target], directory, environment)
                if name == "llvm_numeric" and output.strip().splitlines() != ["5.0", "3"]:
                    raise RuntimeError("数值样例结果错误：" + output)
        smoke = directory / "linux-smoke"
        run([compiler, root / "tests/platform/linux_smoke.tx", "-o", smoke], directory, environment)
        if "linux-platform-ok" not in run([smoke], directory, environment):
            raise RuntimeError("Linux 系统接口验证失败")
        print("Linux 安装验证通过：包内 ELF 依赖、隔离工具路径、中文空格路径、普通/ThinLTO、Unicode、系统与文件")


if __name__ == "__main__":
    main()
