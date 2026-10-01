"""只驱动本次启动的 GUI 示例进程，验证 TX 事件循环到原生控件的完整路径。"""

import ctypes
from ctypes import wintypes
import subprocess
import time

USER32 = ctypes.WinDLL("user32", use_last_error=True)
CALLBACK = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
USER32.EnumWindows.argtypes = [CALLBACK, wintypes.LPARAM]
USER32.EnumChildWindows.argtypes = [wintypes.HWND, CALLBACK, wintypes.LPARAM]
USER32.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
USER32.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
USER32.GetClassNameW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
USER32.SendMessageTimeoutW.argtypes = [
    wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM,
    wintypes.UINT, wintypes.UINT, ctypes.POINTER(ctypes.c_size_t)]
USER32.SendMessageTimeoutW.restype = wintypes.LPARAM


def text(hwnd, *, class_name=False):
    value = ctypes.create_unicode_buffer(4096)
    function = USER32.GetClassNameW if class_name else USER32.GetWindowTextW
    function(hwnd, value, len(value))
    return value.value


def windows(pid, parent=None):
    found = []

    @CALLBACK
    def visit(hwnd, _):
        owner = wintypes.DWORD()
        USER32.GetWindowThreadProcessId(hwnd, ctypes.byref(owner))
        if owner.value == pid:
            found.append(hwnd)
        return True

    if parent:
        USER32.EnumChildWindows(parent, visit, 0)
    else:
        USER32.EnumWindows(visit, 0)
    return found


def send(hwnd, message, wparam=0, lparam=0):
    result = ctypes.c_size_t()
    if not USER32.SendMessageTimeoutW(hwnd, message, wparam, lparam,
                                     2, 2000, ctypes.byref(result)):
        raise AssertionError("GUI 示例消息超时或窗口已关闭")


def wait_for(predicate):
    deadline = time.monotonic() + 10
    while time.monotonic() < deadline:
        value = predicate()
        if value:
            return value
        time.sleep(0.02)
    raise AssertionError("GUI 示例未在限时内产生预期状态")


def check_example(executable, name):
    process = subprocess.Popen([str(executable)], stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE, creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        top = wait_for(lambda: next((hwnd for hwnd in windows(process.pid)
                                     if text(hwnd, class_name=True) == "tx_graphics_window_g1"), None))
        children = wait_for(lambda: windows(process.pid, top) or None)
        edit = next(hwnd for hwnd in children if text(hwnd, class_name=True) == "Edit")
        content = ctypes.create_unicode_buffer("自动验证😀")
        send(edit, 0x000C, 0, ctypes.addressof(content))  # WM_SETTEXT
        label = "问候" if name == "greeting" else "保存"
        button = next(hwnd for hwnd in children if text(hwnd) == label)
        send(button, 0x00F5)  # BM_CLICK
        expected = "你好，自动验证😀" if name == "greeting" else "已应用：自动验证😀，通知已启用。"
        wait_for(lambda: any(text(hwnd) == expected for hwnd in windows(process.pid, top)))
        send(top, 0x0010)  # WM_CLOSE
        output, error = process.communicate(timeout=10)
        assert process.returncode == 0, (output + error).decode("utf-8", errors="replace")
        print(f"{name} 原生输入/点击 → TX 事件 → 控件更新 → 关闭: PASS")
    finally:
        if process.poll() is None:
            process.terminate()
            process.communicate(timeout=5)
