"""7.1/7.2 定向检查：每个场景使用独立的 UTF-8 临时工作目录。"""

from pathlib import Path
import subprocess
import sys
import tempfile


root = Path(__file__).resolve().parents[1]
compiler = root / "tx" / "txc.exe"
source = root / "tests" / "stdlib" / "filesystem_path_extended.tx"
output_dir = root / "tx_build"
program = output_dir / "filesystem_path_extended.exe"
sys.stdout.reconfigure(encoding="utf-8")


def run(command, **kwargs):
    result = subprocess.run(command, capture_output=True, text=True,
                            encoding="utf-8", errors="replace", timeout=30,
                            **kwargs)
    if result.returncode != 0:
        raise AssertionError(result.stdout + result.stderr)
    return result.stdout.strip()


def main():
    output_dir.mkdir(exist_ok=True)
    run([str(compiler), str(source), "-o", str(program)])
    for mode, expected in (
        (0, "FS_METADATA_WATCH_OK"),
        (1, ("FS_SYMLINK_OK", "FS_SYMLINK_CREATE_PERMISSION_DENIED")),
        (2, "PATH_EXTENDED_OK"),
    ):
        with tempfile.TemporaryDirectory(prefix="文件 路径_", dir=output_dir) as work:
            value = run([str(program)], cwd=work, input=f"{mode}\n")
            allowed = (expected,) if isinstance(expected, str) else expected
            assert value in allowed, (mode, value)
            print(value)
    junction = Path("C:/Users/All Users")
    if junction.is_symlink():
        with tempfile.TemporaryDirectory(prefix="文件 路径_", dir=output_dir) as work:
            value = run([str(program)], cwd=work, input="3\n")
            assert value == "FS_EXISTING_JUNCTION_OK", value
            print(value)


if __name__ == "__main__":
    main()
