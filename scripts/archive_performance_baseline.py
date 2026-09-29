"""归档 2026-09-29 性能复测仍留在 tx_build 中的原始证据。"""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys


root = Path(__file__).resolve().parent.parent
archive = root / "benchmarks/performance_baseline_2026-09-29"
bundle = root / "tx_build/performance_baseline_2026-09-29/legacy_bundle"
manifest_path = archive / "manifest.json"

samples = (
    "performance_retest_20260927_runtime_context.json",
    "performance_retest_20260927_committed.json",
    "performance_retest_20260929.json",
    "diverse_performance_results.json",
    "diverse_performance_results_20260929.json",
)
pair_samples = (
    "call_borrowing_same_session.json",
    "static_runtime_same_session.json",
)
legacy_programs = (
    "call_borrowing_committed.exe",
    "static_runtime_committed.exe",
)
runtime_dlls = (
    "libgcc_s_seh-1.dll",
    "libicudt78.dll",
    "libicuin78.dll",
    "libicuuc78.dll",
    "libstdc++-6.dll",
    "libstdc++-u.dll",
    "libwinpthread-1.dll",
    "libwinpthread-u.dll",
    "msquic.dll",
)
source_suffixes = {".cpp", ".java", ".py", ".tx"}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def copy_exact(source: Path, destination: Path) -> dict:
    if not source.is_file():
        raise FileNotFoundError(source)
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, destination)
    source_hash = sha256(source)
    if sha256(destination) != source_hash:
        raise RuntimeError(f"归档复制后哈希不匹配：{source}")
    return {"source": source.relative_to(root).as_posix(),
            "archive": destination.relative_to(root).as_posix(),
            "sha256": source_hash, "bytes": source.stat().st_size}


def command_output(command: list[str]) -> str | None:
    try:
        result = subprocess.run(command, cwd=root, capture_output=True,
                                text=True, encoding="utf-8", errors="replace",
                                timeout=20, check=False)
    except (OSError, subprocess.TimeoutExpired):
        return None
    if result.returncode:
        return None
    output = result.stdout or result.stderr
    return output.splitlines()[0].strip() if output else None


def source_hashes() -> dict[str, str]:
    paths = [root / "scripts/run_diverse_performance.py",
             root / "scripts/build.ps1", root / "benchmarks/diverse_performance.tx",
             root / "benchmarks/diverse_performance.cpp",
             root / "benchmarks/diverse_performance.py",
             root / "benchmarks/diverse_performance_java.java",
             root / "benchmarks/call_borrowing.tx",
             root / "benchmarks/static_runtime.tx"]
    paths.extend(path for path in (root / "benchmarks/language_features").iterdir()
                 if path.is_file() and path.suffix in {".tx", ".txh", ".cpp",
                                                       ".hpp", ".py", ".java", ".ps1"})
    return {path.relative_to(root).as_posix(): sha256(path)
            for path in sorted(paths)}


def git_snapshot() -> dict:
    result = subprocess.run(["git", "diff", "--binary", "--no-ext-diff"],
                            cwd=root, capture_output=True, check=True)
    status = subprocess.run(["git", "status", "--porcelain=v1", "-uall"],
                            cwd=root, capture_output=True, check=True)
    return {
        "head": command_output(["git", "rev-parse", "HEAD"]),
        "tracked_diff_sha256": hashlib.sha256(result.stdout).hexdigest(),
        "tracked_diff_bytes": len(result.stdout),
        "status_paths": status.stdout.decode("utf-8", errors="replace").splitlines(),
    }


def verify() -> None:
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    copied = list(manifest["copied_files"])
    pair_manifest = archive / "pair_samples_manifest.json"
    if pair_manifest.exists():
        copied.extend(json.loads(pair_manifest.read_text(encoding="utf-8"))["copied_files"])
    equivalence_manifest = archive / "equivalence_manifest.json"
    if equivalence_manifest.exists():
        copied.append(json.loads(equivalence_manifest.read_text(
            encoding="utf-8"))["copied_file"])
    verified = 0
    skipped_local = 0
    local_bundle_available = bundle.is_dir()
    for item in copied:
        path = root / item["archive"]
        if not local_bundle_available and path.is_relative_to(bundle):
            skipped_local += 1
            continue
        if not path.is_file() or sha256(path) != item["sha256"]:
            raise RuntimeError(f"归档文件丢失或变化：{path}")
        verified += 1
    print(f"已核对 {verified} 个归档文件的 SHA-256；"
          f"仅本地保存的二进制文件跳过 {skipped_local} 个")


def create() -> None:
    if manifest_path.exists():
        raise FileExistsError("基线清单已存在；使用 --verify，不覆盖历史快照")
    copied = []
    for name in samples:
        copied.append(copy_exact(root / "tx_build" / name,
                                 archive / "samples" / name))
    audit_root = root / "tx_build/perf_audit_20260927"
    for path in sorted(audit_root.iterdir()):
        if path.is_file() and (path.suffix in source_suffixes or path.name == "REPORT.md"):
            copied.append(copy_exact(path, archive / "audit" / path.name))
    copied.append(copy_exact(root / "tx_build/performance_retest_20260927.py",
                             archive / "audit/performance_retest_20260927.py"))
    copied.append(copy_exact(root / "tx_build/language_comparison/compare_languages.py",
                             archive / "audit/compare_languages.py"))
    for name in (*legacy_programs, *runtime_dlls):
        copied.append(copy_exact(root / "tx_build" / name, bundle / name))
    tools = {
        "bundled_clang": command_output([str(root / "tx/clang.exe"), "--version"]),
        "g++": command_output(["g++", "--version"]),
        "python": command_output([sys.executable, "--version"]),
        "java": command_output(["java", "-version"]),
        "node": command_output(["node", "--version"]),
    }
    data = {
        "captured_at_utc": datetime.now(timezone.utc).isoformat(),
        "historical_measurement_date": "2026-09-29",
        "git": git_snapshot(),
        "machine": {"platform": platform.platform(),
                    "processor_identifier": os.environ.get("PROCESSOR_IDENTIFIER"),
                    "reported_cpu": "AMD Ryzen 7 6800H",
                    "reported_os": "Windows 11 Pro build 26200"},
        "toolchain_versions": tools,
        "toolchain_sha256": {name: sha256(root / name) for name in
                              ("tx/txc.exe", "tx/libtxstdlib.a", "tx/clang.exe")},
        "benchmark_source_sha256": source_hashes(),
        "copied_files": copied,
        "legacy_bundle_note": "DLL 是归档时 tx_build 中的文件；09-27 原始配对来源无法独立证实。",
    }
    manifest_path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n",
                             encoding="utf-8")
    verify()


def capture_pairs() -> None:
    pair_manifest = archive / "pair_samples_manifest.json"
    if pair_manifest.exists():
        raise FileExistsError("同轮样本已经归档，不覆盖")
    copied = [copy_exact(root / "tx_build/performance_baseline_2026-09-29" / name,
                         archive / "samples" / name) for name in pair_samples]
    pair_manifest.write_text(json.dumps({
        "captured_at_utc": datetime.now(timezone.utc).isoformat(),
        "copied_files": copied,
        "legacy_dll_provenance": "unknown; see manifest.json legacy_bundle_note",
    }, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    for item in copied:
        if sha256(root / item["archive"]) != item["sha256"]:
            raise RuntimeError(f"同轮样本复制后哈希不匹配：{item['archive']}")
    print(f"已归档 {len(copied)} 份同轮原始样本")


def capture_equivalence() -> None:
    source = root / "tx_build/performance_equivalence/results_complete.json"
    destination = archive / "samples/equivalence_results.json"
    evidence = archive / "equivalence_manifest.json"
    if evidence.exists():
        raise FileExistsError("等价样本已经归档，不覆盖")
    copied = copy_exact(source, destination)
    evidence.write_text(json.dumps({
        "captured_at_utc": datetime.now(timezone.utc).isoformat(),
        "copied_file": copied,
    }, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print("已归档等价对照原始样本")


def main() -> None:
    parser = argparse.ArgumentParser()
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--verify", action="store_true")
    mode.add_argument("--capture-pairs", action="store_true")
    mode.add_argument("--capture-equivalence", action="store_true")
    args = parser.parse_args()
    if args.verify:
        verify()
    elif args.capture_pairs:
        capture_pairs()
    elif args.capture_equivalence:
        capture_equivalence()
    else:
        create()


if __name__ == "__main__":
    main()
