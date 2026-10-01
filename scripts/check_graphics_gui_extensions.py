"""仅验证图形 G2–G3 / GUI U2–U3 及直接受影响的基础行为。"""

from pathlib import Path
import os
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "tx_build/graphics_gui_g2_u3"
TXC = ROOT / "tx/txc.exe"


def run(arguments, timeout=180, expected=0):
    environment = os.environ.copy()
    environment["PATH"] = str(ROOT / "tx") + os.pathsep + environment["PATH"]
    completed = subprocess.run([str(value) for value in arguments], cwd=ROOT,
                               env=environment, capture_output=True, encoding="utf-8",
                               errors="replace", timeout=timeout)
    output = completed.stdout + completed.stderr
    if (completed.returncode == 0) != (expected == 0):
        raise AssertionError(output or str(arguments))
    return output


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    OUTPUT.mkdir(parents=True, exist_ok=True)
    groups = set(sys.argv[1:]) or {"graphics", "gui", "baseline", "native"}
    for folder, name, symbol in (
        ("graphics", "g2_g3", "txrt_graphics_text_hit_test"),
        ("gui", "u2_u3", "txrt_gui_data_apply_page_list_model"),
    ):
        if folder not in groups:
            continue
        source = ROOT / f"tests/{folder}/{name}.tx"
        ir = OUTPUT / f"{name}.ll"
        run([TXC, "emit-llvm", source, "-o", ir])
        code = ir.read_text(encoding="utf-8")
        assert f"call i32 @{symbol}(ptr " in code
        assert "call i32 @txrt_external_call" not in code
        for mode in ("native", "lto"):
            executable = OUTPUT / f"{name}_{mode}.exe"
            run([TXC, source, "-o", executable, *(["--no-lto"] if mode == "native" else [])])
            print(f"{mode}: {run([executable], timeout=30).strip()}", flush=True)
    for folder in ("graphics", "gui"):
        if "baseline" not in groups:
            continue
        executable = OUTPUT / f"{folder}_baseline.exe"
        run([TXC, ROOT / f"tests/{folder}/behavior.tx", "--no-lto", "-o", executable])
        print(run([executable], timeout=30).strip(), flush=True)
        rejected = run([TXC, "check", ROOT / f"tests/{folder}/thread_capture.tx"], expected=1)
        assert ".tx:" in rejected or ".tx(" in rejected
    if "native" in groups:
        executable = OUTPUT / "windows_g2_u3.exe"
        libraries = ["d2d1", "dwrite", "windowscodecs", "imm32", "gdi32", "ole32", "uuid",
                     "uiautomationcore", "oleacc", "oleaut32",
                     "comctl32", "comdlg32", "user32", "shell32", "winhttp", "ws2_32",
                     "dnsapi", "advapi32", "bcrypt", "crypt32", "ncrypt", "iconv"]
        run([shutil.which("g++"), "-std=c++23", "-O1", "-pthread", "-Isrc",
             ROOT / "tests/graphics/windows_g2_u3.cpp", "tx/libtxstdlib.a",
             "tx/link/gui-manifest.o", "-Ltx/link", *[f"-l{name}" for name in libraries],
             "-o", executable])
        print(run([executable], timeout=30).strip(), flush=True)
    print("定向验证通过：" + ", ".join(sorted(groups)), flush=True)


if __name__ == "__main__":
    main()
