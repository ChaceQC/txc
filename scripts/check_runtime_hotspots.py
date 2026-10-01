"""双平台热点修复契约门禁；不使用共享 runner 的计时阈值。"""
import json
import os
import platform
import subprocess
import tempfile
from pathlib import Path

from check_platform import ROOT, TOOL_DIR, TXC, EXE_SUFFIX, CREATE_FLAGS, environment


def native_libraries():
    if os.name == "nt":
        return [f"-L{TOOL_DIR / 'link'}", "-lwinhttp", "-lws2_32", "-ldnsapi",
                "-ladvapi32", "-lbcrypt", "-lcrypt32", "-lncrypt", "-lshell32",
                "-luser32", "-liconv", "-lpsapi"]
    return [f"-L{TOOL_DIR / 'lib'}", "-licui18n", "-licuuc", "-licudata",
            "-lpcre2-8", "-lsodium", "-largon2", "-lxml2", "-lpq", "-lcurl",
            "-lssl", "-lcrypto", "-lcares", "-lz", "-pthread"]


def main():
    report = ROOT / "tx_build/runtime_hotspot_checks/results.json"
    report.parent.mkdir(parents=True, exist_ok=True)
    evidence = {"platform": platform.system(), "passed": False, "checks": []}

    def save():
        report.write_text(json.dumps(evidence, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    def run(name, args, cwd, expected_code=0, input_text=None):
        result = subprocess.run(list(map(str, args)), cwd=cwd, env=environment(),
            input=input_text, capture_output=True, encoding="utf-8", errors="strict",
            timeout=240, creationflags=CREATE_FLAGS)
        evidence["checks"].append({"name": name, "exit_code": result.returncode,
            "stdout": result.stdout[-8000:], "stderr": result.stderr[-8000:]})
        save()
        if result.returncode != expected_code:
            raise RuntimeError(f"{name}: exit={result.returncode}\n{result.stdout[-2000:]}{result.stderr[-2000:]}")
        print(f"PASS {name}\n{result.stdout.strip()}", flush=True)
        return result

    save()
    try:
        with tempfile.TemporaryDirectory(prefix="tx-hotspots-") as directory:
            folder = Path(directory)
            cases = {"json/behavior": "JSON_OK", "serde/direct": "SERDE_DIRECT_TX_OK",
                     "containers/call_effects": "call effects 0", "containers/behavior": None}
            for case, marker in cases.items():
                target = folder / (case.replace("/", "_") + EXE_SUFFIX)
                run(case + " build", [TXC, ROOT / "tests" / (case + ".tx"), "-o", target], ROOT)
                output = run(case, [target], folder).stdout
                if marker and marker not in output.splitlines():
                    raise RuntimeError(case + " 缺少成功标记")
                if case == "containers/behavior":
                    lines = output.splitlines()
                    if len(lines) != 4 or lines[0] != "maps true" or lines[2:] != ["fields true", "order true"] or 'queue<str>["内容!"]' not in lines[1]:
                        raise RuntimeError("类型化容器行为不正确")
            target = folder / ("queue_error" + EXE_SUFFIX)
            run("queue error build", [TXC, ROOT / "tests/containers/errors.tx", "-o", target], ROOT)
            failure = run("queue error cleanup", [target], folder, expected_code=1, input_text="2\n")
            if failure.stdout.splitlines() != ["still alive"] or "空 queue 不能 front" not in failure.stderr:
                raise RuntimeError("空队列异常转换或析构清理不正确")
            native = ["json/stream_native", "serde/direct_native"]
            if os.name != "nt":
                native.append("platform/linux_directory_names")
            for case in native:
                target = folder / (case.replace("/", "_") + EXE_SUFFIX)
                run(case + " build", ["g++", "-std=c++23", "-O2", "-finput-charset=UTF-8",
                    "-fexec-charset=UTF-8", "-Isrc", ROOT / "tests" / (case + ".cpp"),
                    TOOL_DIR / "libtxstdlib.a", *native_libraries(), "-o", target], ROOT)
                run(case, [target], folder)
        evidence["passed"] = True
        print("RUNTIME_HOTSPOTS_OK", flush=True)
    except Exception as error:
        evidence["failure"] = str(error)
        raise
    finally:
        save()


if __name__ == "__main__":
    main()
