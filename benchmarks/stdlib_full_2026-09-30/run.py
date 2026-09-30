"""新增标准库四语言串行配对采样；失败样本不进入报告。"""
from contextlib import nullcontext
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import shutil
import statistics
import subprocess
import sys
import tempfile
import time

from prepare import root, source, work, load
from services import servers

groups = {
    "concurrency": {"thread_spawn_join": 100, "mutex_uncontended": 100000, "atomic_add": 100000,
                    "channel_send_recv": 200010000, "task_spawn_wait": 500},
    "sqlite": {"sqlite_insert": 2000, "sqlite_read": 20150000, "sqlite_savepoint": 100,
               "sqlite_pool": 4200, "sqlite_async": 4200, "migration_recheck": 100},
    "postgres": {"postgres_insert": 2000, "postgres_read": 20150000, "postgres_savepoint": 100},
    "security": {"secret_equal": 20000, "argon2_hash_verify": 3, "ed25519_sign": 32000,
                 "ed25519_verify": 500},
    "network": {"dns_localhost": 100, "udp_echo": 512000, "ipc_echo": 21000, "tls_handshake": 10},
    "async_file": {"async_file_rw": 4194304},
    "diagnostics": {"test_parameterized": 199990000, "test_property": 20000,
                    "log_filtered": 100000, "log_file": 2000},
    "profile": {"profile_spans": 1000},
}


def snapshot():
    paths = [*source.glob("*.py"), *source.glob("*.tx"), *source.glob("*.cpp"),
             *source.glob("*.hpp"), *source.glob("*.java"), *work.glob("*.exe"), *work.glob("*.class"),
             *work.joinpath("dependencies").glob("*.jar"), root / "tx/txc.exe", root / "tx/libtxstdlib.a"]
    paths.extend(root.glob("src/**/*.cpp"))
    paths.extend(root.glob("src/**/*.hpp"))
    paths.extend(root.glob("tx/stdlib/*.txh"))
    paths.extend(root.glob("tx/*.dll"))
    return {str(path.relative_to(root)): hashlib.sha256(path.read_bytes()).hexdigest() for path in sorted(paths)}


def save(name, value):
    (source / name).write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def measure(command, folder, environment, expected):
    start = time.perf_counter()
    result = subprocess.run(list(map(str, command)), cwd=folder, env=environment,
                            capture_output=True, encoding="utf-8", errors="strict", timeout=120)
    if result.returncode:
        message = result.stdout[-1500:] + result.stderr[-1500:]
        for key in ("TX_DB_PASSWORD", "TX_DB_PASSWORD_HEX"):
            if environment.get(key):
                message = message.replace(environment[key], "[REDACTED]")
        raise RuntimeError(f"{Path(command[0]).name} exited {result.returncode}: {message}")
    lines = result.stdout.splitlines()
    if len(lines) == len(expected) and all(len(line.split()) == 3 for line in lines):
        lines = [part for line in lines for part in line.split()]
    if len(lines) != len(expected) * 3:
        raise ValueError("unexpected benchmark output: " + result.stdout[:700])
    cases = {}
    for index in range(0, len(lines), 3):
        name = lines[index]
        elapsed = float(lines[index + 1]) / 1000
        checksum = int(lines[index + 2])
        if name in cases or name not in expected or checksum != expected[name] or elapsed <= 0:
            raise ValueError(f"invalid result {name}: checksum={checksum}, expected={expected.get(name)}, ms={elapsed}")
        cases[name] = {"ms": elapsed, "checksum": checksum}
    if "log_file" in cases:
        records = [json.loads(line) for line in (folder / "events.jsonl").read_text(encoding="utf-8").splitlines()]
        if len(records) != 2000 or any(record["fields"]["index"] != index
                or record["fields"]["password"] != "[REDACTED]"
                or record["context"]["request_id"] != "bench"
                for index, record in enumerate(records, 1)):
            raise ValueError("log content mismatch")
    return {"wall_ms": (time.perf_counter() - start) * 1000, "cases": cases}


def main():
    smoke = "--smoke" in sys.argv[1:]
    calibration = "--calibrate" in sys.argv[1:]
    selected = [arg for arg in sys.argv[1:] if arg not in ("--smoke", "--calibrate")] or list(groups)
    unknown = set(selected) - set(groups)
    if unknown:
        raise ValueError(unknown)
    # 单组参数用于首次负载调试；正式采样使用无参数入口，保留每组原始数据。
    official = (len(selected) == len(groups) or calibration) and not smoke
    prefix = "calibration_" if calibration else ""
    if official and (source / (prefix + "completed.json")).exists():
        raise RuntimeError("refusing to overwrite completed run")
    fixtures = work / "fixtures"
    groups["security"]["x509_parse_der"] = (fixtures / "server.der").stat().st_size * 500
    environment = os.environ.copy()
    environment["PATH"] = str(root / "tx") + os.pathsep + environment["PATH"]
    environment["PYTHONUTF8"] = "1"
    environment["PYTHONPATH"] = str(work / "dependencies/python")
    classpath = os.pathsep.join([str(work), *map(str, (work / "dependencies").glob("*.jar"))])
    before = snapshot()
    if official:
        save(prefix + "manifest.json", {"started_utc": datetime.now(timezone.utc).isoformat(),
            "head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip(),
            "warmups": 1, "rounds": 5, "sha256": before, "groups": groups,
            "policy": "每组每语言独立进程；一轮预热、五轮轮换顺序，内部微秒计时中位数；本机临时TLS数据库和回声服务器"})
    database = load("database_servers", root / "scripts/check_db_postgres.py")
    pg_root = database.server_root() if "postgres" in selected else None
    results = {}
    for group in selected:
        commands = {"TX": [work / (("database" if group in ("sqlite", "postgres") else group) + ".exe")],
                    "C++": [work / "reference_cpp.exe", group],
                    "Java": ["java", "-cp", classpath, "reference_java", group],
                    "Python": [sys.executable, "-X", "utf8", "-B", source / "reference.py", group]}
        with tempfile.TemporaryDirectory(prefix=group + "_", dir=work) as temporary:
            temporary = Path(temporary)
            service = database.temporary_server(temporary, pg_root) if group == "postgres" else (
                servers(fixtures) if group == "network" else nullcontext({}))
            with service as variables:
                env = {**environment, **variables, "BENCH_GROUP": group}
                # 数据库辅助环境自带 PATH，恢复当前发行 DLL 优先级；不输出其余配置。
                env["PATH"] = str(root / "tx") + os.pathsep + env["PATH"]
                samples = {language: [] for language in commands}
                for iteration in range(1 if smoke else 6):
                    labels = list(commands)
                    labels = labels[iteration % 4:] + labels[:iteration % 4]
                    for language in labels:
                        folder = temporary / (language.replace("+", "p") + str(iteration))
                        folder.mkdir()
                        for fixture in fixtures.iterdir():
                            if fixture.suffix not in (".key",):
                                shutil.copy2(fixture, folder / fixture.name)
                        sample = measure(commands[language], folder, env, groups[group])
                        if iteration or smoke:
                            samples[language].append(sample)
                    print(f"PASS {group}: {'warmup' if iteration == 0 else str(iteration) + '/5'}", flush=True)
                cases = {name: {language: {"median_ms": statistics.median(sample["cases"][name]["ms"] for sample in values),
                    "samples_ms": [sample["cases"][name]["ms"] for sample in values], "checksum": checksum}
                    for language, values in samples.items()} for name, checksum in groups[group].items()}
                results[group] = {"cases": cases, "samples": samples}
                save(prefix + "results.json" if official else "debug_results.json", results)
    if official:
        after = snapshot()
        if after != before:
            save("changed_inputs.json", [key for key in before.keys() | after.keys() if before.get(key) != after.get(key)])
            raise RuntimeError("inputs changed during measurement")
        save(prefix + "completed.json", {"completed_utc": datetime.now(timezone.utc).isoformat(),
             "source_toolchain_and_references_unchanged": True, "groups": list(results),
             "cases": sum(len(value["cases"]) for value in results.values()), "languages": ["TX", "C++", "Java", "Python"]})
    print("ALL SELECTED NEW STDLIB SUITES COMPLETE", flush=True)


if __name__ == "__main__":
    main()
