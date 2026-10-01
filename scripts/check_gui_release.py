"""G5/U6 定向检查；只关闭本脚本创建进程的错误框。"""

import ctypes
from ctypes import wintypes
import os
from pathlib import Path
import platform
import struct
import subprocess
import tempfile
import time

from check_ci_package import check_gui_package, run

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "tx_build/gui_release"


def dialog_error(program, environment, expected_log):
    user = ctypes.WinDLL("user32", use_last_error=True)
    callback_type = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    user.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
    user.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
    user.PostMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
    user.EnumWindows.argtypes = [callback_type, wintypes.LPARAM]
    user.EnumChildWindows.argtypes = [wintypes.HWND, callback_type, wintypes.LPARAM]
    startup = subprocess.STARTUPINFO()
    startup.dwFlags = subprocess.STARTF_USESTDHANDLES | subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0
    process = subprocess.Popen([str(program)], cwd=OUT, env=environment,
                               startupinfo=startup, creationflags=subprocess.DETACHED_PROCESS)
    messages = []

    @callback_type
    def collect(hwnd, _):
        text = ctypes.create_unicode_buffer(2048)
        user.GetWindowTextW(hwnd, text, len(text))
        messages.append(text.value)
        return True

    @callback_type
    def find(hwnd, _):
        pid = wintypes.DWORD()
        user.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
        if pid.value == process.pid:
            title = ctypes.create_unicode_buffer(256)
            user.GetWindowTextW(hwnd, title, len(title))
            if title.value == "TX 运行错误":
                user.EnumChildWindows(hwnd, collect, 0)
                user.PostMessageW(hwnd, 0x111, 1, 0)  # 本进程错误框 IDOK
        return True

    try:
        deadline = time.monotonic() + 15
        while process.poll() is None and time.monotonic() < deadline:
            user.EnumWindows(find, 0)
            time.sleep(0.025)
        assert process.wait(timeout=3) == 1
        assert messages, "没有出现无控制台错误框"
        if expected_log:
            logs = list(expected_log.glob("TX/diagnostics/error-*.log"))
            assert len(logs) == 1
            text = logs[0].read_text(encoding="utf-8")
            assert "运行错误" in text and "failure" in text and "release_error.tx" in text
            assert str(logs[0]) in "\n".join(messages)
        else:
            assert "诊断日志保存失败" in "\n".join(messages)
    finally:
        if process.poll() is None:
            process.kill()
            process.wait()


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    windows = Path(environment["SYSTEMROOT"])
    environment["PATH"] = os.pathsep.join(map(str, [windows / "System32", windows]))
    environment.pop("TX_LLVM_BIN", None)
    txc = ROOT / "tx/txc.exe"
    # 两种模式的 GUI 帧/原生控件与 PE/manifest 检查复用 ZIP 的同一入口。
    check_gui_package(ROOT, environment)
    for flags, name in (([], "lto"), (["--no-lto"], "native")):
        program = OUT / (name + "-error.exe")
        run([txc, ROOT / "tests/gui/release_error.tx", "--subsystem", "windows",
             *flags, "-o", program], ROOT, environment)
        result = subprocess.run([str(program)], capture_output=True, env=environment,
                                timeout=15, encoding="utf-8")
        assert result.returncode == 1 and "运行错误" in result.stderr and "failure" in result.stderr
        with tempfile.TemporaryDirectory(prefix="tx-diagnostics-") as temporary:
            folder = Path(temporary)
            blocked = folder / "blocked"
            blocked.write_text("not a directory", encoding="utf-8")
            for index, (local, fallback, expected) in enumerate((
                (folder / "local", folder / "temp", folder / "local"),
                (blocked, folder / "fallback", folder / "fallback"),
                (blocked, blocked, None),
            )):
                case_env = environment | {"LOCALAPPDATA": str(local), "TEMP": str(fallback)}
                dialog_error(program, case_env, expected)
    console = OUT / "console.exe"
    run([txc, ROOT / "examples/llvm_numeric.tx", "--subsystem", "console", "-o", console], ROOT, environment)
    data = console.read_bytes()
    pe = struct.unpack_from("<I", data, 0x3c)[0]
    assert struct.unpack_from("<H", data, pe + 24 + 68)[0] == 3
    assert run([console], ROOT, environment).strip().splitlines() == ["5.0", "3"]
    for arguments in (("--subsystem",), ("--subsystem", "invalid"),
                      ("--subsystem", "windows", "--subsystem", "console")):
        result = subprocess.run([str(txc), str(ROOT / "example.tx"), *arguments],
                                capture_output=True, env=environment, timeout=30)
        assert result.returncode != 0
    print(f"G5/U6 通过：双链接 GUI/清单/重定向/日志/回退/错误框、console、参数拒绝；{platform.platform()}")


if __name__ == "__main__":
    main()
