"""Linux 全量性能补充：平台负载、同轮对照、外部题和启动资源。"""
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import importlib.util
import json
import math
import os
import signal
import statistics
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
archive = root / "benchmarks/linux_full_2026-10-01"
spec = importlib.util.spec_from_file_location("linux_bench", archive / "run.py")
bench = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bench)
bench.save = lambda name, value: (archive / ("extra_" + name)).write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
work = bench.work
command = bench.command


def library_pair(target):
    samples = {"TX": [], "C++": []}
    checks = {}
    for iteration in range(6):
        for label in list(samples) if iteration % 2 == 0 else list(reversed(samples)):
            binary = work / "library" if label == "TX" else target
            output = command([binary])
            values = bench.triples(output, 1)
            current = {k: v["checksum"] for k, v in values.items()}
            if label in checks and current != checks[label]:
                raise ValueError("库对照校验值不稳定")
            checks[label] = current
            if iteration:
                samples[label].append({"cases": values, "stdout": output})
    pairs = [(key, key) for key in checks["TX"] if key in checks["C++"]]
    pairs.extend(("dict", key) for key in ("dict_dynamic", "dict_hash"))
    medians = {}
    for tx_key, cpp_key in pairs:
        if not math.isclose(float(checks["TX"][tx_key]), float(checks["C++"][cpp_key]), rel_tol=1e-12, abs_tol=1e-6):
            raise ValueError("库对照校验不一致: " + tx_key)
        medians[cpp_key] = {label: statistics.median(s["cases"][key]["ms"] for s in samples[label])
                            for label, key in (("TX", tx_key), ("C++", cpp_key))}
    return {"samples": samples, "medians_ms": medians, "checksums": checks,
            "mapping": "TX dict 分别比较 C++ dict_dynamic 与 dict_hash；浮点校验 rel_tol=1e-12 abs_tol=1e-6"}


def references():
    configs = {
        "library": ([root / "benchmarks/library_compare/compare.cpp"], 1),
        "features": ([root / "benchmarks/performance_baseline_2026-09-29/audit/features.cpp"], 1000),
        "diverse": ([root / "benchmarks/diverse_performance.cpp", root / "benchmarks/performance_equivalence/parse_contract.cpp", root / "benchmarks/performance_equivalence/serde_payload.cpp"], 1000),
        "format_contract": ([root / "benchmarks/performance_completion_2026-09-30/format_contract.cpp"], 1000),
    }
    for name, (sources, divisor) in configs.items():
        try:
            target = work / (name + "_cpp")
            command(["g++", "-std=c++23", "-O3", "-DNDEBUG", "-pthread", *sources, "-o", target])
            value = library_pair(target) if name == "library" else bench.measure(name, {"TX": [work / name], "C++": [target]}, {"TX": divisor, "C++": divisor})
            value["status"] = "passed"
            bench.results[name + "_paired"] = value
        except Exception as error:
            bench.results[name + "_paired"] = {"status": "failed", "error": str(error)}
        bench.save("results.json", bench.results)
    value = bench.measure("language_python", {"TX": [work / "language"], "C++": [work / "language_cpp"],
                          "Python": [sys.executable, root / "benchmarks/language_features/compare_py.py"]},
                          {"TX": 1000, "C++": 1, "Python": 1})
    value["status"] = "passed"
    bench.results["language_python_paired"] = value
    bench.save("results.json", bench.results)


def platform_suites():
    audit = root / "benchmarks/performance_baseline_2026-09-29/audit"
    child = work / "child"
    command(["g++", "-O3", audit / "child.cpp", "-o", child])
    source = (audit / "system.tx").read_text(encoding="utf-8")
    source = source.replace("tx_build/perf_audit_20260927/child.exe", str(child))
    source = source.replace("tx_build/perf_audit_20260927/stream_tx.bin", str(work / "stream.bin"))
    target = work / "system.tx"
    target.write_text(source, encoding="utf-8")
    bench.suite("system", target)
    for name, port in (("http", 19790), ("ws", 19791)):
        # 直接启动 Python 子进程，整个进程组由 finally 回收。
        script = root / "tx_build/perf_audit_20260927" / (name + "_server.py")
        process = subprocess.Popen([sys.executable, "-u", str(script)], stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE, text=True, encoding="utf-8", start_new_session=True)
        try:
            if process.stdout.readline().strip() != "READY":
                raise RuntimeError(name + " server failed")
            expected = {"httpx_get": "100", "requests_get": "100"} if name == "http" else {"websocket_echo": "100"}
            bench.suite(name, audit / (name + ".tx"), expected=expected)
        finally:
            if process.poll() is None:
                os.killpg(process.pid, signal.SIGTERM)
            process.wait(timeout=10)


def external():
    tasks = {
        "mini-filesystem": ["fanout-2000", "deep-pwd-2000", "moves-2000", "random-2000-1", "linklong-2000"],
        "not-yet-on-stage": ["all-free-max", "forced-increasing-max", "forced-decreasing-max", "shuffled-tight-max-1", "alternating-tight-max"],
    }
    for project, cases in tasks.items():
        folder = Path("/mnt/e/Project/problems") / project
        source = folder / "src"
        normalize = bytes.splitlines if project == "mini-filesystem" else bytes.split
        binaries = {"TX": work / (project + "_tx"), "C++": work / (project + "_cpp")}
        command([bench.tool / "txc", source / "solution_benchmark.tx", "-o", binaries["TX"]])
        command(["g++", "-std=c++23", "-O3", source / "solution_benchmark.cpp", "-o", binaries["C++"]])
        official = work / (project + "_solution")
        command([bench.tool / "txc", source / "solution.tx", "-o", official])
        count = 0
        for path in sorted((folder / "testdata").glob("*.in")):
            result = subprocess.run([str(official)], input=path.read_bytes(), capture_output=True, timeout=60)
            if result.returncode or normalize(result.stdout) != normalize(path.with_suffix(".out").read_bytes()):
                raise ValueError(project + "/" + path.name + " answer mismatch")
            count += 1
        values = {}
        for name in cases:
            path = folder / "testdata" / (name + ".in")
            data, expected = path.read_bytes(), path.with_suffix(".out").read_bytes()
            samples = {label: [] for label in binaries}
            for iteration in range(6):
                for label in list(binaries) if iteration % 2 == 0 else list(reversed(binaries)):
                    result = subprocess.run([str(binaries[label])], input=data, capture_output=True, timeout=60)
                    if result.returncode or normalize(result.stdout) != normalize(expected):
                        raise ValueError(project + "/" + name + "/" + label + " answer mismatch")
                    if iteration:
                        samples[label].append(float(result.stderr.strip()) / 1000)
            values[name] = {"samples_ms": samples, "medians_ms": {label: statistics.median(rows) for label, rows in samples.items()}}
            print("PASS external", project, name, flush=True)
        bench.results[project] = {"status": "passed", "official_cases": count, "cases": values}
        bench.save("results.json", bench.results)


def startup():
    target = work / "startup"
    command([bench.tool / "txc", root / "benchmarks/minimal_startup.tx", "-o", target])
    output = {}
    for name, executable in (("startup", target), ("language", work / "language"), ("diverse", work / "diverse")):
        samples = []
        for iteration in range(6):
            result = subprocess.run(["/usr/bin/time", "-f", "RESOURCE %e %M %U %S", str(executable)], cwd=root,
                                    capture_output=True, text=True, encoding="utf-8", timeout=120, check=True)
            fields = next(line.split()[1:] for line in result.stderr.splitlines() if line.startswith("RESOURCE "))
            if iteration:
                samples.append(dict(zip(("wall_seconds", "peak_rss_kib", "user_seconds", "system_seconds"), map(float, fields))))
        output[name] = samples
    bench.save("resources.json", output)


def calibration():
    def adapted_command(args, cwd=root, env=None, timeout=240):
        name = Path(args[0]).name
        if name == "diagnostics":
            (Path(cwd) / "events.jsonl").unlink(missing_ok=True)
        output = command(args, cwd, env, timeout)
        if name == "random_long":
            output = "random_long\n" + output
        if name == "diagnostics":
            records = [json.loads(line) for line in (Path(cwd) / "events.jsonl").read_text(encoding="utf-8").splitlines()]
            if len(records) != 2000 or any(record["fields"]["index"] != index or record["fields"]["password"] != "[REDACTED]" or record["context"]["request_id"] != "bench" for index, record in enumerate(records, 1)):
                raise ValueError("日志内容校验失败")
        return output
    bench.command = adapted_command
    groups = {
        "random_long": (1, None), "diagnostics": (1000, None),
    }
    for name, (divisor, env) in groups.items():
        value = bench.measure(name, {"TX": [work / name]}, {"TX": divisor}, cwd=work if name == "diagnostics" else root, env=env)
        value["status"] = "passed"
        bench.results[name + "_calibrated"] = value
        bench.save("results.json", bench.results)
    bench.command = command
    source = root / "benchmarks/performance_completion_2026-09-30/graph_contract.cpp"
    text = source.read_text(encoding="utf-8").replace("#include <windows.h>\n#include <psapi.h>", "#include <sys/resource.h>")
    text = text.replace("PROCESS_MEMORY_COUNTERS memory{};", "rusage memory{};")
    text = text.replace("GetProcessMemoryInfo(GetCurrentProcess(), &memory, sizeof(memory));", "getrusage(RUSAGE_SELF, &memory);")
    text = text.replace("memory.PeakWorkingSetSize", "(memory.ru_maxrss * 1024)")
    text = text.replace("memory.PeakPagefileUsage", "0")
    adapted = work / "graph_contract.cpp"
    adapted.write_text(text, encoding="utf-8")
    target = work / "graph_contract_native"
    command(["g++", "-std=c++23", "-O3", "-Isrc", adapted, bench.tool / "libtxstdlib.a",
             "-L" + str(bench.tool / "lib"), "-Wl,-rpath," + str(bench.tool / "lib"),
             "-licui18n", "-licuuc", "-licudata", "-lpcre2-8", "-lsodium", "-largon2", "-lxml2", "-lpq", "-lcurl", "-lssl", "-lcrypto", "-lcares", "-lz", "-pthread", "-o", target])
    value = bench.measure("graph_contract", {"TX-runtime": [target], "C++": [target, "reference"]}, {"TX-runtime": 1000, "C++": 1000})
    value["status"] = "passed"
    value["note"] = "Linux getrusage 替换 Windows 内存统计；pagefile 字段不适用，置零；比较直接运行时 API。"
    bench.results["graph_contract_native"] = value
    bench.save("results.json", bench.results)


def security():
    sys.path.insert(0, str(archive))
    from fixtures import load
    from cryptography import x509
    from cryptography.hazmat.primitives import serialization
    from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
    database = load(root / "scripts/check_db_postgres.py")
    with tempfile.TemporaryDirectory(prefix="security_", dir=work) as directory:
        folder = Path(directory)
        database.certificates(folder)
        certificate = x509.load_pem_x509_certificate((folder / "server.crt").read_bytes())
        der = certificate.public_bytes(serialization.Encoding.DER)
        (folder / "server.der").write_bytes(der)
        (folder / "ed_seed.bin").write_bytes(Ed25519PrivateKey.generate().private_bytes(serialization.Encoding.Raw, serialization.PrivateFormat.Raw, serialization.NoEncryption()))
        (folder / "message.bin").write_bytes(bytes(range(256)) * 4)
        (folder / "password.bin").write_bytes(os.urandom(24))
        value = bench.measure("security", {"TX": [work / "security"]}, {"TX": 1000}, cwd=folder,
            expected={"secret_equal": "20000", "argon2_hash_verify": "3", "ed25519_sign": "32000", "ed25519_verify": "500", "x509_parse_der": str(len(der) * 500)})
        value["status"] = "passed"
        bench.results["security_calibrated"] = value
        bench.save("results.json", bench.results)


def postgres_suite():
    sys.path.insert(0, str(archive))
    from fixtures import load, postgres
    package = work / "postgres_package"
    package.mkdir(exist_ok=True)
    binary_root = package / "unpacked/usr/lib/postgresql/16/bin"
    if not (binary_root / "initdb").exists():
        command(["apt-get", "download", "postgresql-16"], cwd=package)
        deb = next(package.glob("postgresql-16_*.deb"))
        command(["dpkg-deb", "-x", deb, package / "unpacked"])
    def relocated(args, cwd=root, env=None, timeout=240):
        args = list(args)
        if str(args[0]).startswith("/usr/lib/postgresql/16/bin/"):
            args[0] = binary_root / Path(args[0]).name
        return command(args, cwd, env, timeout)
    database = load(root / "scripts/check_db_postgres.py")
    with postgres(database, relocated) as environment:
        bench.suite("postgres", root / "benchmarks/stdlib_retest_2026-09-30_static_execution/database.tx",
                    cwd=work, env={**os.environ, **environment},
                    expected={"postgres_insert": "2000", "postgres_read": "20150000", "postgres_savepoint": "100"})
    bench.save("postgres_version.json", {"version": command([binary_root / "postgres", "--version"]),
               "package_sha256": {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in package.glob("*.deb")}})


def fingerprints():
    values = bench.snapshot()
    files = [Path(__file__), *root.joinpath("tx_build/perf_audit_20260927").glob("*_server.py")]
    for name in ("mini-filesystem", "not-yet-on-stage"):
        folder = Path("/mnt/e/Project/problems") / name
        files.extend(p for p in (folder / "src").glob("*") if p.suffix in (".tx", ".txh", ".cpp"))
        files.extend((folder / "testdata").glob("*.in"))
        files.extend((folder / "testdata").glob("*.out"))
    values.update({str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in files})
    return values


def main():
    if (archive / "extra_manifest.json").exists():
        raise RuntimeError("拒绝覆盖补充采样")
    primary = json.loads((archive / "manifest.json").read_text(encoding="utf-8"))["sha256"]
    current = bench.snapshot()
    bench.save("primary_end.json", {"unchanged": primary == current,
        "changed": [p for p in primary.keys() | current.keys() if primary.get(p) != current.get(p)],
        "status": "primary interrupted before PostgreSQL: initdb unavailable; failed fixture cases rerun separately"})
    before = fingerprints()
    bench.save("manifest.json", {"started_utc": datetime.now(timezone.utc).isoformat(), "python": sys.version, "sha256": before})
    for name, action in (("calibration", calibration), ("security", security), ("postgres", postgres_suite), ("platform", platform_suites), ("references", references), ("external", external), ("resources", startup)):
        try:
            action()
        except Exception as error:
            bench.results[name + "_error"] = {"status": "failed", "error": str(error)}
            print("FAIL", name, error, flush=True)
            bench.save("results.json", bench.results)
    after = fingerprints()
    bench.save("completed.json", {"completed_utc": datetime.now(timezone.utc).isoformat(), "unchanged": before == after,
        "changed": [p for p in before.keys() | after.keys() if before.get(p) != after.get(p)]})


if __name__ == "__main__":
    main()
