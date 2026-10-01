"""G0–G1 定向验证：静态类型、直接 ABI、两种链接、窗口与错误清理。"""

from pathlib import Path
import os
import shutil
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "tx_build/graphics_g0_g1"
TXC = ROOT / "tx/txc.exe"


def run(arguments, *, expected=0, timeout=120):
    environment = os.environ.copy()
    environment["PATH"] = str(ROOT / "tx") + os.pathsep + environment["PATH"]
    completed = subprocess.run([str(value) for value in arguments], cwd=ROOT,
                               env=environment, capture_output=True, encoding="utf-8",
                               errors="replace", timeout=timeout)
    text = completed.stdout + completed.stderr
    if expected == 0 and completed.returncode != 0:
        raise AssertionError(text)
    if expected != 0 and completed.returncode == 0:
        raise AssertionError("预期失败却成功：" + str(arguments))
    return text


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    OUTPUT.mkdir(parents=True, exist_ok=True)
    for name in ("wrong_type", "forge_resource", "thread_capture", "nested_send"):
        output = run([TXC, "check", ROOT / f"tests/graphics/{name}.tx"], expected=1)
        assert ".tx:" in output or ".tx(" in output, output
        print(f"静态拒绝 {name}: PASS")

    source = ROOT / "tests/graphics/behavior.tx"
    ir = OUTPUT / "behavior.ll"
    run([TXC, "emit-llvm", source, "-o", ir])
    code = ir.read_text(encoding="utf-8")
    assert "call i32 @txrt_graphics_fill_rect(ptr " in code
    assert "call i32 @txrt_graphics_create_window(ptr " in code
    assert "declare i32 @txrt_graphics_fill_rect(ptr, double" in code
    assert "call i32 @txrt_external_call" not in code
    print("图形标量 C ABI 和固定调用符号: PASS")

    for mode in ("native", "lto"):
        target = OUTPUT / f"behavior_{mode}.exe"
        run([TXC, source, "-o", target, *(["--no-lto"] if mode == "native" else [])])
        print(run([target], timeout=30).strip())
        print(f"{mode} 链接与 TX 生命周期: PASS")

    cleanup = OUTPUT / "exception_cleanup.exe"
    run([TXC, ROOT / "tests/graphics/exception_cleanup.tx", "-o", cleanup])
    error = run([cleanup], expected=1, timeout=15)
    assert "exception_cleanup.tx" in error and "颜色" in error, error
    print("未捕获错误源码位置与进程退出: PASS")

    target = OUTPUT / "windows_lifecycle.exe"
    run([shutil.which("g++"), "-std=c++23", "-O1", "-pthread", "-Isrc",
         ROOT / "tests/graphics/windows_lifecycle.cpp", "tx/libtxstdlib.a", "-Ltx/link",
         "-ld2d1", "-lgdi32", "-lole32", "-luuid", "-luser32", "-lshell32",
         "-lwinhttp", "-lws2_32", "-ldnsapi", "-ladvapi32", "-lbcrypt", "-lcrypt32",
         "-lncrypt", "-liconv", "-o", target])
    print(run([target], timeout=30).strip())
    run([TXC, ROOT / "examples/graphics/two_windows.tx", "-o", OUTPUT / "two_windows.exe"])
    print("双窗口交互示例构建: PASS（人工桌面验收待执行）")


if __name__ == "__main__":
    main()
