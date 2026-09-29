"""只验证 12.1 公共契约和 12.2 同步 SQLite，可按小项分别执行。"""

from pathlib import Path
from contextlib import closing
import os
import shutil
import sqlite3
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]
COMPILER = ROOT / "tx/txc.exe"
OUTPUT = ROOT / "tx_build"


def run(command, folder, *, expected=0, timeout=60):
    environment = os.environ.copy()
    environment["PATH"] = str(ROOT / "tx") + os.pathsep + environment["PATH"]
    result = subprocess.run(
        [str(part) for part in command], cwd=folder, env=environment,
        capture_output=True, encoding="utf-8", errors="strict", timeout=timeout,
    )
    output = result.stdout + result.stderr
    if result.returncode != expected:
        raise AssertionError(f"exit={result.returncode}\n{output}")
    return output


def compile_and_run(source, folder):
    target = folder / (Path(source).stem + ".exe")
    run([COMPILER, ROOT / source, "-o", target], ROOT, timeout=180)
    output = run([target], folder, timeout=15)
    print(f"PASS {source}: {output.strip()}")


def contracts(folder):
    compile_and_run("tests/db/values.tx", folder)
    for source, marker in (("wrong_parameter.tx", "db.bind"), ("wrong_send.tx", "assert_send")):
        output = run([COMPILER, "check", ROOT / "tests/db" / source], ROOT, expected=1)
        if source not in output or marker not in output:
            raise AssertionError(output)
        print(f"PASS 静态诊断 {source}")
    print("12.1 公共契约定向验证：3/3")


def sqlite_checks(folder):
    with closing(sqlite3.connect(folder / "external.sqlite")) as connection:
        connection.execute("CREATE TABLE dirty(value TEXT)")
        connection.execute("INSERT INTO dirty VALUES(CAST(x'ff' AS TEXT))")
        connection.commit()
    compile_and_run("examples/sqlite.tx", folder)
    compile_and_run("tests/db/sqlite.tx", folder)
    compile_and_run("tests/db/sqlite_errors.tx", folder)
    compile_and_run("tests/db/sqlite_file.tx", folder)
    with closing(sqlite3.connect(folder / "数据 数据库.sqlite")) as connection:
        assert connection.execute("PRAGMA journal_mode").fetchone()[0] == "wal"
        assert connection.execute("SELECT value FROM values_table ORDER BY value").fetchall() == [(1,), (2,)]
    assert not (folder / "does_not_exist.sqlite").exists()
    print("PASS Python SQLite 互操作：WAL、提交结果和关闭回滚")
    native = folder / "thread_and_cleanup.exe"
    compiler = shutil.which("g++")
    if not compiler:
        raise RuntimeError("未找到 g++")
    run([compiler, "-std=c++23", "-O0", "-pthread", "-Isrc",
         ROOT / "tests/db/thread_and_cleanup.cpp", "tx/libtxstdlib.a", "-Ltx/link",
         "-lwinhttp", "-lws2_32", "-ldnsapi", "-ladvapi32", "-lbcrypt",
         "-lcrypt32", "-lncrypt", "-lshell32", "-luser32", "-liconv",
         "-o", native], ROOT, timeout=90)
    print(run([native], folder, timeout=15).strip())
    print("12.2 SQLite 定向验证：6/6")


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    selected = sys.argv[1:] or ["contracts", "sqlite"]
    if any(name not in ("contracts", "sqlite") for name in selected):
        raise SystemExit("用法：python scripts/check_db.py [contracts] [sqlite]")
    OUTPUT.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="db_", dir=OUTPUT) as temporary:
        folder = Path(temporary)
        for group in selected:
            (contracts if group == "contracts" else sqlite_checks)(folder)


if __name__ == "__main__":
    main()
