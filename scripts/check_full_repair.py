"""仅验证本轮所有权、错误、格式化及证书复用涉及的行为。"""
from datetime import datetime, timezone
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
archive = root / "benchmarks/performance_full_repair_2026-09-30"
environment = {**os.environ, "PATH": str(root / "tx") + os.pathsep + os.environ["PATH"]}


def run(command, folder, expected=0):
    result = subprocess.run(list(map(str, command)), cwd=folder, env=environment,
                            capture_output=True, encoding="utf-8", timeout=180)
    if result.returncode != expected:
        raise RuntimeError(result.stdout + result.stderr)
    return result.stdout + result.stderr


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cases", nargs="*")
    parser.add_argument("--scripts", nargs="*")
    parser.add_argument("--name", default="checks")
    args = parser.parse_args()
    results = {}
    cases = ["performance_full_repair", "static_borrowing_lifetime", "stdlib/sync_behavior",
             "stdlib/channel_behavior", "diagnostics/property_test", "diagnostics/log_extended_test",
             "strings/format_optimized", "strings/regex_behavior", "serde/behavior", "serde/direct",
             "performance_12_14/graphs", "performance_completion_gc", "bytes_file_stream/encodings"]
    if args.cases is not None:
        cases = args.cases
    selected_scripts = args.scripts
    output_name = args.name
    with tempfile.TemporaryDirectory(prefix="tx-full-repair-") as temporary:
        folder = Path(temporary)
        for case in cases:
            executable = folder / (case.replace("/", "_") + ".exe")
            run([root / "tx/txc.exe", root / "tests" / (case + ".tx"), "-o", executable], root)
            results[case] = run([executable], folder).strip()
            print("PASS " + case, flush=True)
        ir = folder / "repair.ll"
        run([root / "tx/txc.exe", "emit-llvm", root / "tests/performance_full_repair.tx", "-o", ir], root)
        text = ir.read_text(encoding="utf-8")
        body = re.search(r"define i64 @tx_fn_[^(]*unpack_snapshot_0\([^\n]*\).*?^}", text, re.M | re.S)
        assert body and "call i32 @txrt_value_snapshot_box(" not in body.group()
        assert "@txrt_channel_recv_required_i64(" in text and "@txrt_log_enabled_literal(" in text
        results["ir"] = "unpack snapshot stays scalar; required recv and literal log paths emitted"
    for script, script_args in [("check_x509.py", []), ("check_concurrency_errors.py", []),
                         ("check_db.py", ["contracts", "sqlite"]), ("check_package_compatibility.py", [])]:
        if selected_scripts is not None and script not in selected_scripts:
            continue
        results[script] = run(["python", "-X", "utf8", "-B", root / "scripts" / script, *script_args], root).strip()
        print("PASS " + script, flush=True)
    output = {"completed_utc": datetime.now(timezone.utc).isoformat(), "checks": results,
              "compiler_sha256": hashlib.sha256((root / "tx/txc.exe").read_bytes()).hexdigest(),
              "stdlib_sha256": hashlib.sha256((root / "tx/libtxstdlib.a").read_bytes()).hexdigest()}
    archive.mkdir(parents=True, exist_ok=True)
    path = archive / (output_name + ".json")
    if path.exists():
        raise RuntimeError("refusing to overwrite checks")
    path.write_text(json.dumps(output, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
