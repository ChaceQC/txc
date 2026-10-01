"""GUI U0–U1 定向验证；不运行标准库全量测试。"""

from pathlib import Path
import os
import shutil
import subprocess
import sys
from check_gui_examples import check_example

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "tx_build/gui_u0_u1"
TXC = ROOT / "tx/txc.exe"


def run(arguments, *, expected=0, timeout=120):
    environment = os.environ.copy()
    environment["PATH"] = str(ROOT / "tx") + os.pathsep + environment["PATH"]
    completed = subprocess.run([str(value) for value in arguments], cwd=ROOT,
                               env=environment, capture_output=True, encoding="utf-8",
                               errors="replace", timeout=timeout)
    text = completed.stdout + completed.stderr
    if (expected == 0) != (completed.returncode == 0):
        raise AssertionError(text or str(arguments))
    return text


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    OUTPUT.mkdir(parents=True, exist_ok=True)
    for name in ("wrong_type", "forge_resource", "thread_capture", "nested_send"):
        output = run([TXC, "check", ROOT / f"tests/gui/{name}.tx"], expected=1)
        assert ".tx:" in output or ".tx(" in output, output
        print(f"静态拒绝 {name}: PASS")
    source = ROOT / "tests/gui/behavior.tx"
    ir = OUTPUT / "behavior.ll"
    run([TXC, "emit-llvm", source, "-o", ir])
    code = ir.read_text(encoding="utf-8")
    assert "call i32 @txrt_gui_set_text_text_box(ptr " in code
    assert "call i32 @txrt_gui_set_height_button(ptr " in code
    assert "call i32 @txrt_gui_set_grid(ptr " in code
    assert "call i32 @txrt_external_call" not in code
    print("具体控件符号和标量布局 ABI: PASS")
    for mode in ("native", "lto"):
        target = OUTPUT / f"behavior_{mode}.exe"
        run([TXC, source, "-o", target, *(["--no-lto"] if mode == "native" else [])])
        print(run([target], timeout=30).strip())
        print(f"{mode} 生命周期与属性: PASS")
    target = OUTPUT / "windows_behavior.exe"
    run([shutil.which("g++"), "-std=c++23", "-O1", "-pthread", "-Isrc",
         ROOT / "tests/gui/windows_behavior.cpp", "tx/libtxstdlib.a",
         "tx/link/gui-manifest.o", "-Ltx/link", "-ld2d1", "-lgdi32", "-lole32",
         "-ldwrite", "-lwindowscodecs", "-limm32", "-lcomdlg32",
         "-luuid", "-lcomctl32", "-luser32", "-lshell32", "-lwinhttp", "-lws2_32",
         "-ldnsapi", "-ladvapi32", "-lbcrypt", "-lcrypt32", "-lncrypt", "-liconv",
         "-o", target])
    print(run([target], timeout=30).strip())
    for name in ("greeting", "settings"):
        run([TXC, ROOT / f"examples/gui/{name}.tx", "-o", OUTPUT / f"{name}.exe"])
        check_example(OUTPUT / f"{name}.exe", name)


if __name__ == "__main__":
    main()
