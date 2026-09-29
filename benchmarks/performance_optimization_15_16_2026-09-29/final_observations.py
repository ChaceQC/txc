"""文件 I/O 保护项和峰值工作集观察；不重跑完整性能套件。"""
import json
import os
from pathlib import Path
import subprocess
import time

import psutil

from measure import archive, digest, out, root, run, sample, save


def memory(program):
    environment = os.environ.copy()
    environment["PATH"] = str(root / "tx") + os.pathsep + environment["PATH"]
    process = psutil.Popen([str(program)], cwd=root, env=environment,
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    peak = 0
    peak_commit = 0
    while process.poll() is None:
        try:
            info = process.memory_info()
            peak = max(peak, info.peak_wset)
            peak_commit = max(peak_commit, info.peak_pagefile)
        except psutil.NoSuchProcess:
            pass
        time.sleep(0.002)
    stdout, stderr = process.communicate()
    assert process.returncode == 0, stderr.decode("utf-8")
    return {"sampled_peak_working_set": peak, "sampled_peak_commit": peak_commit,
            "output_lines": len(stdout.splitlines())}


def main():
    programs = {}
    for label, package in (("old", out / "baseline/tx"), ("new", root / "tx")):
        destination = out / "file_io" / label
        destination.mkdir(parents=True, exist_ok=True)
        executable = destination / "file_io.exe"
        run(package / "txc.exe", archive / "file_io.tx", "-o", executable)
        programs[label] = executable
    result = sample(programs)
    assert result["checksums"] == {"old": {"file_stream_rw": 12800}, "new": {"file_stream_rw": 12800}}
    result["source_sha256"] = digest(archive / "file_io.tx")
    result["memory_sampling_ms"] = 2
    result["memory_samples"] = {
        label: [memory(out / folder / "paths.exe") for _ in range(3)]
        for label, folder in (("old", "baseline"), ("new", "candidate"))}
    save("observations.json", result)
    print(json.dumps({"file_io": result["medians_ms"], "memory": result["memory_samples"]}, indent=2))


if __name__ == "__main__":
    main()
