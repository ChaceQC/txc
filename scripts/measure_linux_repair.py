"""同机旧/新程序配对复测；区分本地文件系统和 Windows 挂载盘。"""
from pathlib import Path
import importlib.util
import json
import os
import shutil
import subprocess
import sys
import tempfile
import statistics

root = Path(__file__).resolve().parents[1]
archive = root / "benchmarks/linux_repair_2026-10-01"
old = root / "tx_build/linux_full_2026_10_01"
tool = root / "tx/linux"


def load(path):
    spec = importlib.util.spec_from_file_location(path.stem, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def run(args, cwd=root):
    result = subprocess.run(list(map(str, args)), cwd=cwd, capture_output=True, encoding="utf-8", timeout=240)
    if result.returncode:
        raise RuntimeError(f"{Path(args[0]).name}: {result.stderr[-1500:]}")
    return result.stdout


def main():
    stage = sys.argv[1]
    archive.mkdir(exist_ok=True)
    output = archive / (stage + ".json")
    if output.exists():
        raise RuntimeError("拒绝覆盖已有采样")
    results = {}
    sources = {
        "library": "benchmarks/library_compare/compare.tx",
        "security": "benchmarks/stdlib_retest_2026-09-30_static_execution/security.tx",
        "diagnostics": "benchmarks/stdlib_retest_2026-09-30_static_execution/diagnostics.tx",
        "async_file": "benchmarks/stdlib_retest_2026-09-30_static_execution/async_file.tx",
        "concurrency": "benchmarks/stdlib_retest_2026-09-30_static_execution/concurrency.tx",
        "ws": "benchmarks/performance_baseline_2026-09-29/audit/ws.tx",
    } if stage.startswith("platform") else {
        "diverse": "benchmarks/diverse_performance.tx",
        "features": "benchmarks/performance_baseline_2026-09-29/audit/features.tx",
        "language": "benchmarks/language_features/compare.tx",
    }
    if stage == "queue":
        sources = {"features": sources["features"]}
    if stage == "serde_final":
        sources = {"diverse": sources["diverse"]}
    with tempfile.TemporaryDirectory(prefix="tx_repair_", dir=root / "tx_build" if stage.endswith("_mount") else None) as directory:
        folder = Path(directory)
        (folder / "tx_build").mkdir()
        shutil.copytree(root / "tx/stdlib", folder / "tx/stdlib")
        fixtures = load(root / "scripts/check_db_postgres.py")
        fixtures.certificates(folder)
        from cryptography import x509
        from cryptography.hazmat.primitives import serialization
        from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
        cert = x509.load_pem_x509_certificate((folder / "server.crt").read_bytes())
        (folder / "server.der").write_bytes(cert.public_bytes(serialization.Encoding.DER))
        (folder / "ed_seed.bin").write_bytes(Ed25519PrivateKey.generate().private_bytes(serialization.Encoding.Raw, serialization.PrivateFormat.Raw, serialization.NoEncryption()))
        (folder / "message.bin").write_bytes(bytes(range(256)) * 4)
        (folder / "block.bin").write_bytes(bytes(range(256)) * 256)
        (folder / "password.bin").write_bytes(os.urandom(24))
        for name, source in sources.items():
            target = folder / (name + "_new")
            run([tool / "txc", root / source, "-o", target])
            previous = folder / (name + "_old")
            shutil.copy2(old / name, previous)
            programs = {"old": previous, "new": target}
            if stage in ("runtime", "queue", "serde_final"):
                cpp = folder / (name + "_cpp")
                cpp_sources = list((root / "benchmarks/language_features").glob("*.cpp")) if name == "language" else (
                    [root / "benchmarks/diverse_performance.cpp", root / "benchmarks/performance_equivalence/parse_contract.cpp", root / "benchmarks/performance_equivalence/serde_payload.cpp"] if name == "diverse" else
                    [root / "benchmarks/performance_baseline_2026-09-29/audit/features.cpp"])
                run(["g++", "-std=c++23", "-O3", "-DNDEBUG", "-pthread", *cpp_sources, "-o", cpp])
                programs["cpp"] = cpp
            server = None
            if name == "ws":
                server = subprocess.Popen([sys.executable, "-u", str(root / "tx_build/perf_audit_20260927/ws_server.py")], stdout=subprocess.PIPE, text=True)
                if server.stdout.readline().strip() != "READY":
                    raise RuntimeError("WS 服务启动失败")
            try:
                samples = {label: [] for label in programs}
                expected = None
                for iteration in range(6):
                    for label in (list(samples) if iteration % 2 == 0 else list(reversed(samples))):
                        (folder / "events.jsonl").unlink(missing_ok=True)
                        stdout = run([programs[label]], cwd=folder)
                        lines = stdout.splitlines()
                        rows = [line.split() for line in lines] if all(len(line.split()) == 3 for line in lines) else [lines[i:i+3] for i in range(0, len(lines), 3)]
                        divisor = 1 if name == "library" or (name == "language" and label == "cpp") else 1000
                        values = {key: [float(elapsed) / divisor, checksum] for key, elapsed, checksum in rows}
                        checks = {key: value[1] for key, value in values.items()}
                        if expected is None:
                            expected = checks
                        if expected != checks:
                            raise RuntimeError(name + " checksum mismatch")
                        if iteration:
                            samples[label].append(values)
                    print(f"PASS {name} {iteration}/5", flush=True)
                results[name] = {"samples": samples, "medians_ms": {key: {label: statistics.median(row[key][0] for row in rows) for label, rows in samples.items()} for key in expected}, "checksums": expected}
                if stage == "queue":
                    results[name]["queue_abi_calls"] = {label: sum("call" in line and "<txrt_queue_" in line for line in run(["objdump", "-d", binary]).splitlines()) for label, binary in programs.items() if label != "cpp"}
                output.write_text(json.dumps(results, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
            finally:
                if server:
                    server.terminate()
                    server.wait(timeout=10)
    print("PAIR COMPLETE", flush=True)


if __name__ == "__main__":
    main()
