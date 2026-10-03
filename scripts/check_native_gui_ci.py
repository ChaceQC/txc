"""运行已交付自绘 GUI 的 CI 门禁，保存逐项结果；不运行 Unicode 官方全量用例。"""

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import time

import check_native_gui as native


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--font", required=True)
    parser.add_argument("--shaping-font", required=True)
    parser.add_argument("--tool-dir", type=Path, default=native.TOOL_DIR)
    options = parser.parse_args()
    for font in (options.font, options.shaping_font):
        if not Path(font).is_file():
            parser.error(f"验证字体不存在：{font}")
    output = native.OUTPUT / "ci"
    output.mkdir(parents=True, exist_ok=True)
    report = {"platform": sys.platform, "passed": False, "checks": []}
    environment = os.environ.copy()
    environment["TXC_TOOL_DIR"] = str(options.tool_dir.resolve())
    environment["TX_GUI_FONT"] = options.font
    environment["PYTHONUTF8"] = "1"
    python = sys.executable
    cases = [
        ("retained", [python, "scripts/check_native_gui.py", "--font", options.font]),
        ("containers_data", [python, "scripts/check_native_gui_extended.py", "--font", options.font]),
        ("interaction", [python, "scripts/check_native_gui_interaction.py", "--font", options.font]),
        ("shaping_abi", [python, "scripts/check_native_gui_shaping.py", "--font", options.shaping_font]),
        ("system", [python, "scripts/check_native_gui_system.py", "--font", options.font]),
        ("accessibility_dialogs", [python, "scripts/check_native_gui_system.py", "--font", options.font, "--only", "protocol"]),
    ]
    if not native.WINDOWS:
        cases.append(("portal_peer", ["dbus-run-session", "--", python, "scripts/check_native_gui_system.py",
                                      "--font", options.font, "--only", "portal"]))

    def save():
        (output / "results.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    save()
    try:
        for name, command in cases:
            started = time.monotonic()
            print(f"GUI CI: {name}", flush=True)
            result = subprocess.run(command, cwd=native.ROOT, env=environment, capture_output=True,
                                    encoding="utf-8", errors="replace", timeout=900)
            log = result.stdout + result.stderr
            (output / f"{name}.log").write_text(log, encoding="utf-8")
            report["checks"].append({"name": name, "exit_code": result.returncode,
                                     "seconds": round(time.monotonic() - started, 2), "log": f"{name}.log"})
            save()
            print(log, end="" if log.endswith("\n") else "\n", flush=True)
            if result.returncode:
                raise RuntimeError(f"GUI CI 检查失败：{name}")
        report["passed"] = True
    except Exception as error:
        report["failure"] = str(error)
        raise
    finally:
        save()
    print("NATIVE_GUI_CI_OK", flush=True)


if __name__ == "__main__":
    main()
