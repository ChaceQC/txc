"""只检查实际 ZIP：离开源码树及构建工具 PATH 后编译、链接并运行。"""

from pathlib import Path
import hashlib
import os
import subprocess
import tempfile
import zipfile


root = Path(__file__).resolve().parents[1]


def run(arguments, directory, environment):
    result = subprocess.run([str(value) for value in arguments], cwd=directory,
                            env=environment, capture_output=True, text=True,
                            encoding="utf-8", errors="replace", timeout=180)
    if result.returncode != 0:
        raise RuntimeError(f"运行失败：{arguments}\n{result.stdout}\n{result.stderr}")
    return result.stdout


def main():
    archives = list((root / "dist").glob("*.zip"))
    if len(archives) != 1:
        raise RuntimeError("本轮构建必须只生成一个发布 ZIP")
    archive = archives[0]
    expected = (root / "dist/SHA256SUMS.txt").read_text(encoding="utf-8").split()[0]
    with archive.open("rb") as stream:
        if hashlib.file_digest(stream, "sha256").hexdigest() != expected:
            raise RuntimeError("ZIP 的 SHA-256 不匹配")
    with tempfile.TemporaryDirectory(prefix="txc 安装验证 ") as temporary:
        installed = Path(temporary)
        with zipfile.ZipFile(archive) as package:
            if package.testzip() is not None:
                raise RuntimeError("ZIP 校验失败")
            package.extractall(installed)
        directory = installed / archive.stem
        for name in ("README.md", "docs/usage.md", "docs/syntax.md", "build-info.json"):
            if not (directory / name).is_file():
                raise RuntimeError(f"解压包缺少 {name}")
        environment = os.environ.copy()
        windows = Path(environment["SYSTEMROOT"])
        environment["PATH"] = os.pathsep.join(str(path) for path in (windows / "System32", windows))
        environment.pop("TX_LLVM_BIN", None)
        txc = directory / "tx/txc.exe"
        run([txc, "check", directory / "example.tx"], directory, environment)
        # 两条链接路径及 ICU DLL，覆盖打包最容易遗漏的真实依赖。
        for source, flags in (("llvm_numeric.tx", []), ("llvm_numeric.tx", ["--no-lto"]),
                              ("unicode.tx", []), ("unicode.tx", ["--no-lto"])):
            program = directory / ("smoke-native.exe" if flags else "smoke.exe")
            run([txc, directory / "examples" / source, *flags, "-o", program],
                directory, environment)
            output = run([program], directory, environment)
            if source == "llvm_numeric.tx" and output.strip().splitlines() != ["5.0", "3"]:
                raise RuntimeError(f"数值样例输出不符：{output}")
        print("ZIP 安装验证通过：中文/空格路径、语法检查、ThinLTO、普通链接和 ICU 运行时")


if __name__ == "__main__":
    main()
