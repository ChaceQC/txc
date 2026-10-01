"""本次目录和 JSON/serde 热路径修改的定向边界验证。"""
from pathlib import Path
import os
import json
import sys
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
tool = root / "tx/linux"
evidence = []


def run(args, cwd):
    env = {**os.environ, "LD_LIBRARY_PATH": str(tool / "lib"), "TXC_TOOL_DIR": str(tool)}
    result = subprocess.run(list(map(str, args)), cwd=cwd, env=env, capture_output=True,
                            encoding="utf-8", timeout=240)
    if result.returncode:
        raise RuntimeError(result.stdout[-2000:] + result.stderr[-2000:])
    print(result.stdout.strip(), flush=True)
    if result.stdout.strip():
        evidence.append(result.stdout.strip())
    return result.stdout


with tempfile.TemporaryDirectory(prefix="tx-repair-check-") as directory:
    folder = Path(directory)
    for case in ("json/behavior", "serde/direct", "containers/call_effects", "containers/behavior"):
        target = folder / case.replace("/", "_")
        run([tool / "txc", root / "tests" / (case + ".tx"), "-o", target], root)
        output = run([target], folder)
        if case == "containers/behavior":
            lines = output.splitlines()
            if len(lines) != 4 or lines[0] != "maps true" or lines[2:] != ["fields true", "order true"] or 'queue<str>["内容!"]' not in lines[1]:
                raise RuntimeError("类型化队列的值/复制/顺序检查失败")
    target = folder / "queue_error"
    run([tool / "txc", root / "tests/containers/errors.tx", "-o", target], root)
    failure = subprocess.run([str(target)], input="2\n", cwd=folder, capture_output=True, encoding="utf-8", timeout=30)
    if failure.returncode != 1 or failure.stdout.splitlines() != ["still alive"] or "空 queue 不能 front" not in failure.stderr:
        raise RuntimeError("空队列异常转换或析构清理不正确")
    evidence.append("QUEUE_EMPTY_CLEANUP_OK")
    for case in ("json/stream_native", "serde/direct_native", "platform/linux_directory_names"):
        target = folder / case.replace("/", "_")
        run(["g++", "-std=c++23", "-O2", "-Isrc", root / "tests" / (case + ".cpp"),
             tool / "libtxstdlib.a", "-L" + str(tool / "lib"),
             "-licui18n", "-licuuc", "-licudata", "-lpcre2-8", "-lsodium", "-largon2", "-lxml2", "-lpq", "-lcurl", "-lssl", "-lcrypto", "-lcares", "-lz", "-pthread", "-o", target], root)
        run([target], folder)
    run([sys.executable, root / "scripts/check_x509.py"], root)
    run([sys.executable, root / "scripts/check_linux_network.py", "--cases", "ws"], root)

archive = root / "benchmarks/linux_repair_2026-10-01"
archive.mkdir(exist_ok=True)
(archive / "checks.json").write_text(json.dumps({"passed": True, "outputs": evidence}, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
