"""GUI U4–U5 定向验证；仅操作本脚本启动的窗口，不修改系统主题或剪贴板。"""

from pathlib import Path
import ctypes
from ctypes import wintypes
import os
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "tx_build/gui_u4_u5"
TXC = ROOT / "tx/txc.exe"
ENV = os.environ.copy()
ENV["PATH"] = str(ROOT / "tx") + os.pathsep + ENV["PATH"]


def run(arguments, expected=0, timeout=180):
    completed = subprocess.run([str(arg) for arg in arguments], cwd=ROOT, env=ENV,
                               capture_output=True, encoding="utf-8", errors="replace", timeout=timeout)
    text = completed.stdout + completed.stderr
    if (completed.returncode == 0) != (expected == 0):
        raise AssertionError(text or str(arguments))
    return text


def task_smoke(executable, complete):
    user = ctypes.WinDLL("user32", use_last_error=True)
    callback_type = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    user.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
    user.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
    user.PostMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
    user.EnumWindows.argtypes = [callback_type, wintypes.LPARAM]
    user.EnumChildWindows.argtypes = [wintypes.HWND, callback_type, wintypes.LPARAM]
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0
    process = subprocess.Popen([str(executable)], cwd=ROOT, env=ENV, startupinfo=startup,
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    try:
        window = None
        deadline = time.monotonic() + 15
        while time.monotonic() < deadline:
            windows = []

            @callback_type
            def find(hwnd, _):
                pid = wintypes.DWORD()
                user.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
                if pid.value == process.pid:
                    title = ctypes.create_unicode_buffer(256)
                    user.GetWindowTextW(hwnd, title, len(title))
                    if title.value == "TX 任务工作台":
                        windows.append(hwnd)
                return True

            user.EnumWindows(find, 0)
            if windows:
                window = windows[0]
                break
            if process.poll() is not None:
                raise AssertionError(process.communicate())
            time.sleep(0.025)
        assert window, "工作台未创建窗口"
        if complete:
            done = False
            while time.monotonic() < deadline:
                labels = []

                @callback_type
                def label(hwnd, _):
                    text = ctypes.create_unicode_buffer(256)
                    user.GetWindowTextW(hwnd, text, len(text))
                    labels.append(text.value)
                    return True

                user.EnumChildWindows(window, label, 0)
                if "计算完成" in labels:
                    done = True
                    break
                time.sleep(0.05)
                if process.poll() is not None:
                    raise AssertionError(process.communicate())
            assert done, f"后台任务没有完成或 UI 未更新：{labels}"
        # 工作期间关闭必须先取消、持续泵送，再收束 worker 和窗口。
        user.PostMessageW(window, 0x0010, 0, 0)
        stdout, stderr = process.communicate(timeout=10)
        assert process.returncode == 0, (stdout + stderr).decode("utf-8", errors="replace")
    finally:
        if process.poll() is None:
            process.kill()
            process.communicate()


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    OUTPUT.mkdir(parents=True, exist_ok=True)
    groups = set(sys.argv[1:]) or {"tx", "native", "example"}
    if "tx" in groups:
        for source in ("u4_wrong_type", "u4_thread_capture"):
            diagnostic = run([TXC, "check", ROOT / f"tests/gui/{source}.tx"], expected=1)
            assert ".tx:" in diagnostic or ".tx(" in diagnostic, diagnostic
        ir = OUTPUT / "u4_u5.ll"
        run([TXC, "emit-llvm", ROOT / "tests/gui/u4_u5.tx", "-o", ir])
        code = ir.read_text(encoding="utf-8")
        assert "call i32 @txrt_gui_begin_canvas(ptr " in code
        assert "call i32 @txrt_gui_set_accessibility_canvas(ptr " in code
        assert "call i32 @txrt_external_call" not in code
        for mode in ("native", "lto"):
            executable = OUTPUT / f"u4_u5_{mode}.exe"
            run([TXC, ROOT / "tests/gui/u4_u5.tx", "-o", executable,
                 *(["--no-lto"] if mode == "native" else [])])
            print(f"{mode}: {run([executable], timeout=30).strip()}", flush=True)
    if "native" in groups:
        executable = OUTPUT / "windows_u4_u5.exe"
        libraries = ["d2d1", "dwrite", "windowscodecs", "imm32", "gdi32", "ole32", "uuid",
                     "uiautomationcore", "oleacc", "oleaut32", "comctl32", "comdlg32", "user32",
                     "shell32", "winhttp", "ws2_32", "dnsapi", "advapi32", "bcrypt", "crypt32", "ncrypt", "iconv"]
        run([shutil.which("g++"), "-std=c++23", "-O1", "-pthread", "-Isrc",
             ROOT / "tests/gui/windows_u4_u5.cpp", "tx/libtxstdlib.a", "tx/link/gui-manifest.o",
             "-Ltx/link", *[f"-l{name}" for name in libraries], "-o", executable])
        print(run([executable], timeout=30).strip(), flush=True)
    if "example" in groups:
        executable = OUTPUT / "task_workspace.exe"
        run([TXC, ROOT / "examples/gui/task_workspace.tx", "-o", executable])
        task_smoke(executable, False)
        task_smoke(executable, True)
        print("任务示例：运行中关闭/取消收束、正常完成后关闭通过", flush=True)
    print("GUI U4–U5 定向验证通过：" + ", ".join(sorted(groups)), flush=True)


if __name__ == "__main__":
    main()
