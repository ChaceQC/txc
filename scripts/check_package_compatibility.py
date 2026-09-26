"""对已构建的工具链做少量兼容指纹定向验证。"""

from pathlib import Path
import shutil
import subprocess
import tempfile


PROJECT_ROOT = Path(__file__).resolve().parent.parent
TOOL_DIR = PROJECT_ROOT / "tx"
SOURCE = PROJECT_ROOT / "examples" / "parse_errors.tx"


def check(package: Path, expected: str | None) -> None:
    result = subprocess.run(
        [str(package / "txc.exe"), "check", str(SOURCE)],
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        check=False,
    )
    output = result.stdout + result.stderr
    if expected is None:
        assert result.returncode == 0, output
    else:
        assert result.returncode != 0 and expected in output, output


def main() -> None:
    with tempfile.TemporaryDirectory(prefix="tx_compat_") as temporary:
        package = Path(temporary)
        for name in ("txc.exe", "libtxstdlib.a", "package.compat"):
            shutil.copy2(TOOL_DIR / name, package / name)
        for dependency in TOOL_DIR.glob("*.dll"):
            shutil.copy2(dependency, package / dependency.name)
        shutil.copytree(TOOL_DIR / "stdlib", package / "stdlib")

        check(package, None)

        header = package / "stdlib" / "error.txh"
        header.write_bytes(header.read_bytes() + b"\n# mismatch\n")
        check(package, "标准库接口 与当前工具链不匹配")
        shutil.copy2(TOOL_DIR / "stdlib" / "error.txh", header)

        manifest = package / "package.compat"
        original = manifest.read_text(encoding="utf-8")
        fields = original.splitlines()
        fields[1] = "abi " + "0" * 64
        manifest.write_text("\n".join(fields) + "\n", encoding="utf-8")
        check(package, "运行时 ABI 指纹不匹配")
        manifest.write_text(original, encoding="utf-8")

        compiler = package / "txc.exe"
        compiler.write_bytes(compiler.read_bytes() + b"\x00")
        check(package, "txc 与当前工具链不匹配")
        shutil.copy2(TOOL_DIR / "txc.exe", compiler)

        library = package / "libtxstdlib.a"
        library.write_bytes(library.read_bytes() + b"\x00")
        check(package, "标准库静态库 与当前工具链不匹配")

    print("兼容指纹正常与 4 种错配场景通过")


if __name__ == "__main__":
    main()
