"""本次新增容器、视觉编辑和数据视图的定向检查，不运行其他标准库测试。"""

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
        parser.error("请通过 --font 指定用于验证的 TrueType 字体")
    sys.stdout.reconfigure(encoding="utf-8")
    native.TOOL_DIR = options.tool_dir.resolve()
    native.TXC = native.TOOL_DIR / ("txc.exe" if native.WINDOWS else "txc")
    native.FONT = options.font
    native.OUTPUT.mkdir(parents=True, exist_ok=True)
    if options.only != "abi":
        native.native_check("extended_behavior.cpp", "extended")
    if options.only == "behavior":
        return
    source = native.ROOT / "tests/native_gui/extended_behavior.tx"
    ir = native.OUTPUT / ("extended_windows.ll" if native.WINDOWS else "extended_linux.ll")
    native.run([native.TXC, "emit-llvm", source, "-o", ir])
    code = ir.read_text(encoding="utf-8")
    for symbol in ("create_scroll", "create_tabs", "create_split", "canvas_fill_rect", "replace_items_table_view"):
        assert f"call i32 @txrt_native_gui_{symbol}(" in code, symbol
    assert "call i32 @txrt_external_call" not in code
    for lto in (False, True):
        suffix = "windows.exe" if native.WINDOWS else "linux"
        target = native.OUTPUT / f"extended_{'lto' if lto else 'native'}_{suffix}"
        native.run([native.TXC, source, "-o", target, *([] if lto else ["--no-lto"])])
        native.run([target])
        print(f"新增容器 / 数据 / 画布静态 ABI {'ThinLTO' if lto else '普通'}: PASS")


if __name__ == "__main__":
    main()
