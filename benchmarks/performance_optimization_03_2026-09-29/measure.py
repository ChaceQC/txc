"""同会话交替测量 02 与 03 的调用专项，保留逐轮校验值。"""

from datetime import datetime, timezone
from hashlib import sha256
from pathlib import Path
from statistics import median
import json
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[2]
OLD = ROOT / "tx_build/call_borrowing_02.exe"
NEW = ROOT / "tx_build/call_borrowing_03.exe"
SOURCE = ROOT / "benchmarks/call_borrowing.tx"
OUTPUT = Path(__file__).with_name("samples.json")


def digest(path):
    return sha256(path.read_bytes()).hexdigest()


def run(path):
    result = subprocess.run(
        [str(path)], cwd=ROOT, capture_output=True,
        encoding="utf-8", errors="strict", timeout=60,
    )
    if result.returncode:
        raise RuntimeError(f"{path.name}: {result.stdout}{result.stderr}")
    values = {}
    for line in result.stdout.splitlines():
        name, elapsed, checksum = line.split()
        values[name] = {"ms": float(elapsed) / 1000.0, "checksum": checksum}
    if set(values) != {"dictionary", "map", "set", "queue", "cancel"}:
        raise RuntimeError(f"unexpected output: {result.stdout}")
    return values


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    paths = {"02": OLD, "03": NEW}
    for path in (*paths.values(), SOURCE):
        if not path.is_file():
            raise FileNotFoundError(path)
    warmup = {label: run(path) for label, path in paths.items()}
    samples = []
    for index in range(5):
        order = ("02", "03") if index % 2 == 0 else ("03", "02")
        for label in order:
            samples.append({"round": index + 1, "program": label,
                            "values": run(paths[label])})
    checksums = {
        name: {sample["values"][name]["checksum"] for sample in samples}
        for name in warmup["02"]
    }
    if any(len(values) != 1 for values in checksums.values()):
        raise RuntimeError(f"checksum mismatch: {checksums}")
    summary = {}
    for name in checksums:
        old = median(sample["values"][name]["ms"] for sample in samples
                     if sample["program"] == "02")
        new = median(sample["values"][name]["ms"] for sample in samples
                     if sample["program"] == "03")
        summary[name] = {"02_median_ms": old, "03_median_ms": new,
                         "02_over_03": old / new, "checksum": next(iter(checksums[name]))}
    record = {
        "measured_at_utc": datetime.now(timezone.utc).isoformat(),
        "source_sha256": digest(SOURCE),
        "program_sha256": {label: digest(path) for label, path in paths.items()},
        "warmup": warmup, "samples": samples, "summary": summary,
    }
    OUTPUT.write_text(json.dumps(record, ensure_ascii=False, indent=2) + "\n",
                      encoding="utf-8")
    for name, values in summary.items():
        print(name, values)
    print(f"原始样本：{OUTPUT}")


if __name__ == "__main__":
    main()
