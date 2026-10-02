"""针对本轮文字与 XIM 改动的检查；单独编译自研源码，不链接其他标准库模块。"""

from pathlib import Path
import argparse
import os
import shutil
import subprocess
import sys


root = Path(__file__).resolve().parents[1]
windows = os.name == "nt"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--font", default="C:/Windows/Fonts/msyh.ttc" if windows else "")
    parser.add_argument("--fallback-font", default="C:/Windows/Fonts/segoeui.ttf" if windows else "")
    parser.add_argument("--only", choices=("line", "layout", "buffer", "xim", "bidi", "shaping"))
    parser.add_argument("--shaping-font", default="C:/Windows/Fonts/arial.ttf" if windows else "")
    options = parser.parse_args()
    if options.only == "xim" and windows:
        parser.error("XIM 协议检查需要 Linux X11/XWayland")
    if options.only in (None, "layout") and not options.font:
        parser.error("布局检查需要 --font 指定 TrueType 字体")
    if options.only == "shaping" and not options.shaping_font:
        parser.error("整形检查需要 --shaping-font 指定具有阿拉伯特性的 TrueType 字体")
    sys.stdout.reconfigure(encoding="utf-8")
    compiler = shutil.which("g++" if windows else "clang++-18")
    if not compiler:
        parser.error("未找到 C++23 编译器")
    output = root / "tx_build/native_gui"
    output.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    environment["PATH"] = str(root / ("tx" if windows else "tx/linux")) + os.pathsep + environment.get("PATH", "")
    suites = {
        "shaping": ("opentype_behavior.cpp", [options.shaping_font, output / ("shaping_windows.bmp" if windows else "shaping_linux.bmp")]),
        "bidi": ("bidi_behavior.cpp", [root / "tests/native_gui/data/bidi_test.txt",
                                      root / "tests/native_gui/data/bidi_character_test.txt"]),
        "line": ("line_break_behavior.cpp", [root / "tests/native_gui/data/line_break_test.txt"]),
        "layout": ("text_layout_behavior.cpp", [options.font, *([options.fallback_font] if options.fallback_font else [])]),
        "buffer": ("text_buffer_behavior.cpp", []),
    }
    if not windows:
        suites["xim"] = ("xim_behavior.cpp", [])
    for name, (source, arguments) in suites.items():
        if options.only and options.only != name:
            continue
        if name == "shaping" and not options.shaping_font:
            continue
        target = output / (Path(source).stem + (".exe" if windows else "_linux"))
        sources = [root / "tests/native_gui" / source]
        core = root / "src/stdlib/native_gui/core"
        platform = root / "src/stdlib/native_gui/platform"
        common = [core / (name + ".cpp") for name in
                  ("unicode", "unicode_properties", "grapheme", "word_boundary")]
        if name == "xim":
            sources.append(root / "tests/native_gui/xim_fixture.cpp")
            sources += [*common, *sorted(platform.glob("xim_*.cpp")), *sorted(platform.glob("x11_*.cpp"))]
        elif name == "bidi":
            sources += [core / (name + ".cpp") for name in ("bidi", "bidi_sequence", "bidi_properties")]
        elif name == "line":
            sources += [core / "line_break.cpp", core / "line_break_rules.cpp"]
        elif name == "buffer":
            sources += [*common, core / "text_buffer.cpp", core / "text_navigation.cpp",
                        core / "line_break.cpp", core / "line_break_rules.cpp"]
        else:
            sources += sorted(core.glob("*.cpp"))
            if name == "shaping":
                sources += [root / "tests/native_gui/opentype_substitution_behavior.cpp",
                            root / "tests/native_gui/opentype_position_behavior.cpp"]
        command = [compiler, "-std=c++23", "-O1", "-Wall", "-Wextra", "-Wpedantic", "-Isrc"]
        if windows:
            command += ["-finput-charset=UTF-8", "-fexec-charset=UTF-8"]
        command += [*sources, "-o", target]
        subprocess.run([str(value) for value in command], cwd=root, env=environment, check=True, timeout=120)
        subprocess.run([str(target), *map(str, arguments)], cwd=root, env=environment, check=True, timeout=30)


if __name__ == "__main__":
    main()
