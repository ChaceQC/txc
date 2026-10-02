"""下拉框、访问键与默认/取消动作的定向核心和静态 ABI 检查。"""

import argparse
from pathlib import Path
import sys

import check_native_gui as native


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--font", default=native.FONT)
    parser.add_argument("--tool-dir", type=Path, default=native.TOOL_DIR)
    parser.add_argument("--only", choices=("behavior", "abi"))
    options = parser.parse_args()
    if not options.font:
        parser.error("请通过 --font 指定验证字体")
    sys.stdout.reconfigure(encoding="utf-8")
    native.TOOL_DIR = options.tool_dir.resolve()
    native.TXC = native.TOOL_DIR / ("txc.exe" if native.WINDOWS else "txc")
    native.FONT = options.font
    native.OUTPUT.mkdir(parents=True, exist_ok=True)
    if options.only != "abi":
        native.native_check("interaction_behavior.cpp", "interaction")
    if options.only == "behavior":
        return
    source = native.ROOT / "tests/native_gui/interaction_behavior.tx"
    suffix = "windows.exe" if native.WINDOWS else "linux"
    ir = native.OUTPUT / ("interaction_windows.ll" if native.WINDOWS else "interaction_linux.ll")
    native.run([native.TXC, "emit-llvm", source, "-o", ir])
    code = ir.read_text(encoding="utf-8")
    for symbol in ("create_combo_box", "set_combo_items", "set_access_key_combo_box", "set_default_button", "set_cancel_button"):
        assert f"call i32 @txrt_native_gui_{symbol}(" in code, symbol
    assert "call i32 @txrt_external_call" not in code
    for lto in (False, True):
        target = native.OUTPUT / f"interaction_{'lto' if lto else 'native'}_{suffix}"
        native.run([native.TXC, source, "-o", target, *([] if lto else ["--no-lto"])])
        native.run([target])
        print(f"基础交互静态 ABI {'ThinLTO' if lto else '普通'}: PASS")
    example = native.ROOT / "examples/native_gui/interaction_workbench.tx"
    target = native.OUTPUT / ("interaction_workbench.exe" if native.WINDOWS else "interaction_workbench_linux")
    native.run([native.TXC, example, "-o", target])
    print(f"同源工作台已生成：{target}")


if __name__ == "__main__":
    main()
