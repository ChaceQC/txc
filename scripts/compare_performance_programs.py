"""同会话交替比较两个输出三行一组结果的 TX 程序。"""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import statistics
import subprocess


root = Path(__file__).resolve().parent.parent


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def read_program(path: Path) -> dict[str, tuple[float, str]]:
    result = subprocess.run([str(path)], cwd=root, capture_output=True,
                            encoding="utf-8", errors="strict", timeout=180,
                            check=True)
    lines = result.stdout.splitlines()
    measurements = {}
    if all(len(line.split()) == 3 for line in lines):
        rows = [line.split() for line in lines]
    elif len(lines) % 3 == 0:
        rows = [lines[index:index + 3] for index in range(0, len(lines), 3)]
    else:
        raise ValueError(f"基准结果格式不正确：{path}")
    for name, elapsed, checksum in rows:
        if name in measurements:
            raise ValueError(f"重复项目：{name}")
        measurements[name] = (float(elapsed) / 1000, checksum)
    return measurements


def check(reference: dict, measurement: dict) -> None:
    if set(reference) != set(measurement):
        raise ValueError("新旧程序的项目集合不同")
    for name, (_, checksum) in reference.items():
        if measurement[name][1] != checksum:
            raise ValueError(f"新旧程序校验值不同：{name}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--old", type=Path, required=True)
    parser.add_argument("--new", type=Path, required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--rounds", type=int, choices=(5, 6, 7), default=5)
    args = parser.parse_args()
    paths = {"old": args.old.resolve(), "new": args.new.resolve(),
             "source": args.source.resolve()}
    before = {name: sha256(path) for name, path in paths.items()}
    reference = read_program(paths["old"])
    check(reference, read_program(paths["new"]))
    samples = {"old": {name: [] for name in reference},
               "new": {name: [] for name in reference}}
    order_by_round = []
    for index in range(args.rounds):
        order = ("old", "new") if index % 2 == 0 else ("new", "old")
        order_by_round.append(order)
        for label in order:
            measurement = read_program(paths[label])
            check(reference, measurement)
            for name, (elapsed_ms, _) in measurement.items():
                samples[label][name].append(elapsed_ms)
    after = {name: sha256(path) for name, path in paths.items()}
    if before != after:
        raise RuntimeError("测量期间源码或程序发生变化")
    report = {
        "measured_at_utc": datetime.now(timezone.utc).isoformat(),
        "source_and_program_sha256": after,
        "paths": {name: str(path) for name, path in paths.items()},
        "warmups": 1, "rounds": args.rounds,
        "order_by_round": order_by_round,
        "timing": "program internal microseconds, converted to milliseconds",
        "results": {name: {"checksum": checksum,
                           "old_samples_ms": samples["old"][name],
                           "new_samples_ms": samples["new"][name],
                           "old_median_ms": statistics.median(samples["old"][name]),
                           "new_median_ms": statistics.median(samples["new"][name])}
                    for name, (_, checksum) in reference.items()},
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n",
                           encoding="utf-8")
    print(f"已保存 {len(reference)} 项 × {args.rounds} 轮：{args.output}")


if __name__ == "__main__":
    main()
