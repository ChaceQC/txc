"""自绘 GUI 定向验证：双平台静态 ABI、保留控件、布局和渲染。"""

from pathlib import Path
import argparse
import os
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
WINDOWS = os.name == "nt"
OUTPUT = ROOT / "tx_build/native_gui"
TOOL_DIR = ROOT / ("tx" if WINDOWS else "tx/linux")
TXC = TOOL_DIR / ("txc.exe" if WINDOWS else "txc")
FONT = "C:/Windows/Fonts/msyh.ttc" if WINDOWS else ""


def run(arguments, expected=0):
    environment = os.environ.copy()
    environment["PATH"] = str(TOOL_DIR) + os.pathsep + environment["PATH"]
    if FONT:
        environment["TX_GUI_FONT"] = FONT
    result = subprocess.run([str(value) for value in arguments], cwd=ROOT, env=environment,
                            capture_output=True, encoding="utf-8", errors="replace", timeout=120)
    output = result.stdout + result.stderr
    if (result.returncode == 0) != (expected == 0):
        raise RuntimeError(output or str(arguments))
    return output


def native_check(source="retained_behavior.cpp", name="retained"):
    target = OUTPUT / (f"{name}_windows.exe" if WINDOWS else f"{name}_linux")
    compiler = shutil.which("g++" if WINDOWS else "clang++-18")
    if WINDOWS:
        libraries = ["tx/link/gui-manifest.o", "-Ltx/link", "-ld2d1", "-lgdi32", "-lole32",
                     "-ldwrite", "-lwindowscodecs", "-limm32", "-lcomdlg32", "-luuid",
                     "-luiautomationcore", "-loleacc", "-loleaut32", "-lcomctl32", "-luser32",
                     "-lshell32", "-lwinhttp", "-lws2_32", "-ldnsapi", "-ladvapi32",
                     "-lbcrypt", "-lcrypt32", "-lncrypt", "-liconv"]
    else:
        libraries = ["-L" + str(TOOL_DIR / "lib"), "-Wl,-rpath," + str(TOOL_DIR / "lib"),
                     "-licui18n", "-licuuc", "-licudata", "-lpcre2-8", "-lsodium", "-largon2",
                     "-lxml2", "-lpq", "-lcurl", "-lssl", "-lcrypto", "-lcares", "-lz", "-ldl"]
    run([compiler, "-std=c++23", "-O1", "-pthread", "-Isrc",
         * (["-finput-charset=UTF-8", "-fexec-charset=UTF-8"] if WINDOWS else []),
         ROOT / "tests/native_gui" / source, TOOL_DIR / "libtxstdlib.a",
         *libraries, "-o", target])
    print(run([target, FONT, OUTPUT / (f"{name}_windows.bmp" if WINDOWS else f"{name}_linux.bmp")]).strip())


def main():
    global TOOL_DIR, TXC, FONT
    parser = argparse.ArgumentParser()
    parser.add_argument("--tool-dir", type=Path, default=TOOL_DIR)
    parser.add_argument("--font", default=FONT)
    arguments = parser.parse_args()
    TOOL_DIR = arguments.tool_dir.resolve()
    TXC = TOOL_DIR / ("txc.exe" if WINDOWS else "txc")
    FONT = arguments.font or os.environ.get("TX_GUI_FONT", "")
    if not FONT:
        parser.error("请通过 --font 指定用于双平台验证的同一 TrueType 字体")
    sys.stdout.reconfigure(encoding="utf-8")
    OUTPUT.mkdir(parents=True, exist_ok=True)
    for name in ("wrong_type", "thread_capture"):
        diagnostic = run([TXC, "check", ROOT / f"tests/native_gui/{name}.tx"], expected=1)
        assert ".tx:" in diagnostic or ".tx(" in diagnostic, diagnostic
        assert ("Send" in diagnostic or "线程" in diagnostic) if name == "thread_capture" else "重载" in diagnostic
        print(f"静态拒绝 {name}: PASS")
    source = ROOT / "tests/native_gui/behavior.tx"
    ir = OUTPUT / ("behavior_windows.ll" if WINDOWS else "behavior_linux.ll")
    run([TXC, "emit-llvm", source, "-o", ir])
    code = ir.read_text(encoding="utf-8")
    assert "call i32 @txrt_native_gui_set_text_button(ptr " in code
    assert "call i32 @txrt_native_gui_set_grid(ptr " in code
    assert "call i32 @txrt_external_call" not in code
    for mode in ("native", "lto"):
        name = f"behavior_{'windows' if WINDOWS else 'linux'}_{mode}"
        target = OUTPUT / (name + (".exe" if WINDOWS else ""))
        run([TXC, source, "-o", target, *(["--no-lto"] if mode == "native" else [])])
        run([target])
        print(f"{mode} TX 静态 ABI / 网格 / 渲染 / 生命周期: PASS")
    native_check()
    run([TXC, ROOT / "examples/native_gui/workbench.tx",
         *(["--subsystem", "windows"] if WINDOWS else []),
         "-o", OUTPUT / ("workbench.exe" if WINDOWS else "workbench_linux")])
    print("跨平台自绘工作台构建: PASS")
    run([TXC, ROOT / "examples/native_gui/editor.tx",
         *(["--subsystem", "windows"] if WINDOWS else []),
         "-o", OUTPUT / ("editor.exe" if WINDOWS else "editor_linux")])
    print("自研编辑与范围控件示例构建: PASS")


if __name__ == "__main__":
    main()
