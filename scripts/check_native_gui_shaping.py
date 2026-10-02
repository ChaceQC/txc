"""仅验证新整形链的普通/ThinLTO 静态 ABI 与同源工作台构建。"""

import argparse
import sys
import check_native_gui as native


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--font", default="C:/Windows/Fonts/arial.ttf" if native.WINDOWS else "")
    options = parser.parse_args()
    if not options.font:
        parser.error("请通过 --font 指定带阿拉伯整形表的 TrueType 字体")
    sys.stdout.reconfigure(encoding="utf-8")
    native.FONT = options.font
    native.OUTPUT.mkdir(parents=True, exist_ok=True)
    platform = "windows" if native.WINDOWS else "linux"
    source = native.ROOT / "tests/native_gui/shaping_behavior.tx"
    bitmap = native.OUTPUT / "shaping_abi.bmp"
    previous = None
    for lto in (False, True):
        mode = "lto" if lto else "native"
        target = native.OUTPUT / f"shaping_{platform}_{mode}{'.exe' if native.WINDOWS else ''}"
        native.run([native.TXC, source, "-o", target, *([] if lto else ["--no-lto"])])
        native.run([target])
        image = bitmap.read_bytes()
        if previous is not None and previous != image:
            raise RuntimeError("普通与 ThinLTO ABI 生成图像不同")
        previous = image
        print(f"{platform} OpenType {mode} ABI/render PASS")
    native.run([native.TXC, native.ROOT / "examples/native_gui/shaping_workbench.tx",
                *(["--subsystem", "windows"] if native.WINDOWS else []), "-o",
                native.OUTPUT / ("shaping_workbench.exe" if native.WINDOWS else "shaping_workbench_linux")])
    print(f"{platform} shaping workbench built")


if __name__ == "__main__":
    main()
