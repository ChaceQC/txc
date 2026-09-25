"""Focused vector checks: native behavior, module contracts and error cleanup."""

from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parents[1]
COMPILER = ROOT / "tx" / "txc.exe"
OUTPUT = ROOT / "tx_build" / "vector_checks"


def compile_example(name):
    target = OUTPUT / f"{name}.exe"
    result = subprocess.run(
        [str(COMPILER), str(ROOT / "examples" / f"{name}.tx"), "-o", str(target)],
        capture_output=True, encoding="utf-8", errors="strict", timeout=30,
    )
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    return target


def run(target, data=""):
    return subprocess.run(
        [str(target)], input=data, capture_output=True,
        encoding="utf-8", errors="strict", timeout=10,
    )


def main():
    result = run(compile_example("vector_operations"))
    assert result.returncode == 0, result.stderr
    assert result.stdout.splitlines() == ["true"] * 15, result.stdout
    print("vector operations: 15/15")
    target = compile_example("vector_error_cleanup")
    for mode in range(6):
        result = run(target, f"{mode}\n")
        assert result.returncode == 1, (mode, result.returncode, result.stderr)
        assert result.stdout.splitlines() == ["still alive"], (mode, result.stdout)
        assert "运行错误" in result.stderr, (mode, result.stderr)
    print("vector errors and destructor cleanup: 6/6")
    for source in sorted((ROOT / "tests" / "vectors").glob("*.tx")):
        result = subprocess.run(
            [str(COMPILER), "check", str(source)], capture_output=True,
            encoding="utf-8", errors="strict", timeout=10,
        )
        assert result.returncode != 0, source.name
        assert source.name in result.stderr, (source.name, result.stderr)
        assert "错误" in result.stderr, (source.name, result.stderr)
    print("vector static type diagnostics: 4/4")
    result = run(compile_example("string_fast_paths"))
    assert result.returncode == 0, result.stderr
    assert result.stdout.splitlines() == ["true"] * 16, result.stdout
    print("string borrowing, UTF-8 and composite keys: 16/16")


if __name__ == "__main__":
    main()
