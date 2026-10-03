"""P6/P7 定向行为、直接 ABI 和同源示例构建。"""

import argparse
from pathlib import Path
import sys

import check_native_gui as native


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--font", default=native.FONT)
    parser.add_argument("--tool-dir", type=Path, default=native.TOOL_DIR)
    parser.add_argument("--only", choices=("behavior", "abi", "protocol", "portal"))
    options = parser.parse_args()
    sys.stdout.reconfigure(encoding="utf-8")
    native.TOOL_DIR = options.tool_dir.resolve()
    native.TXC = native.TOOL_DIR / ("txc.exe" if native.WINDOWS else "txc")
    native.FONT = options.font
    native.OUTPUT.mkdir(parents=True, exist_ok=True)
    if options.only == "portal":
        if native.WINDOWS:
            parser.error("Portal 对端检查只在隔离的 Linux D-Bus 会话运行")
        native.native_check("portal_dialog_behavior.cpp", "portal_dialog")
        return
    if options.only == "protocol":
        source = "uia_behavior.cpp" if native.WINDOWS else "atspi_behavior.cpp"
        native.native_check(source, "uia" if native.WINDOWS else "atspi")
        if native.WINDOWS:
            native.native_check("windows_dialog_behavior.cpp", "windows_dialog")
        return
    if options.only != "abi":
        native.native_check("system_behavior.cpp", "system")
    if options.only == "behavior":
        return
    source = native.ROOT / "tests/native_gui/system_behavior.tx"
    platform = "windows.exe" if native.WINDOWS else "linux"
    ir = native.OUTPUT / f"system_{platform}.ll"
    native.run([native.TXC, "emit-llvm", source, "-o", ir])
    code = ir.read_text(encoding="utf-8")
    for name in ("create_command", "create_menu", "show_modal", "set_accessibility_button", "set_theme"):
        assert f"call i32 @txrt_native_gui_{name}(" in code, name
    assert "call i32 @txrt_external_call" not in code
    for lto in (False, True):
        target = native.OUTPUT / f"system_{'lto' if lto else 'native'}_{platform}"
        native.run([native.TXC, source, "-o", target, *([] if lto else ["--no-lto"])])
        native.run([target])
        print(f"P6/P7 {'ThinLTO' if lto else '普通'} ABI: PASS")
    example = native.ROOT / "examples/native_gui/system_workbench.tx"
    target = native.OUTPUT / ("system_workbench.exe" if native.WINDOWS else "system_workbench_linux")
    native.run([native.TXC, example, "-o", target])
    print(f"系统集成工作台：{target}")


if __name__ == "__main__":
    main()
