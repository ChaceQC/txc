#define WINVER 0x0A00
#define _WIN32_WINNT 0x0A00
#include "stdlib/native_gui/platform/windows_window.hpp"
#include "stdlib/native_gui/core/unicode.hpp"

#include <chrono>
#include <stdexcept>

namespace tx::ui
{
namespace
{
LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) noexcept
{
    auto* window = reinterpret_cast<windows_window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE)
    {
        window = static_cast<windows_window*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);
        window->hwnd = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    }
    if (window)
    {
        try
        {
            return window->message(message, wparam, lparam);
        }
        catch (...)
        {
            // 异常不能穿过系统回调。退出消息循环，让调用方观察窗口关闭。
            PostMessageW(hwnd, WM_CLOSE, 0, 0);
        }
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}
}

windows_window::windows_window(const std::string& title, unsigned width, unsigned height)
{
    if (!width || !height || width > 16384 || height > 16384)
    {
        throw std::invalid_argument("窗口尺寸非法");
    }
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    WNDCLASSEXW type{};
    type.cbSize = sizeof(type);
    type.lpfnWndProc = window_proc;
    type.hInstance = GetModuleHandleW(nullptr);
    type.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    type.lpszClassName = L"tx_self_render_window";
    if (!RegisterClassExW(&type) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        throw std::runtime_error("无法注册自研 GUI 窗口");
    }
    const auto text = utf16_text(decode_utf8(title).scalars);
    const auto dpi = GetDpiForSystem();
    RECT bounds{0, 0, MulDiv(width, dpi, 96), MulDiv(height, dpi, 96)};
    AdjustWindowRectExForDpi(&bounds, WS_OVERLAPPEDWINDOW, FALSE, 0, dpi);
    hwnd = CreateWindowExW(0, type.lpszClassName, text.c_str(), WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, bounds.right - bounds.left, bounds.bottom - bounds.top,
        nullptr, nullptr, type.hInstance, this);
    if (!hwnd)
    {
        throw std::runtime_error("无法创建自研 GUI 窗口");
    }
    scale_ = GetDpiForWindow(hwnd) / 96.0;
    enable_ime(false);
}

windows_window::~windows_window()
{
    close();
}

void windows_window::close() noexcept
{
    if (hwnd)
    {
        DestroyWindow(hwnd);
        hwnd = nullptr;
    }
}

bool windows_window::is_open() const noexcept
{
    return hwnd != nullptr;
}

double windows_window::scale() const noexcept
{
    return scale_;
}

std::intptr_t windows_window::wait_handle() const noexcept
{
    return 0;
}

int windows_window::next_wakeup_ms() const noexcept
{
    return events_.empty() ? -1 : 0;
}

void wait_platform_events(std::span<platform_window* const> windows, int timeout_ms)
{
    for (const auto* window : windows)
    {
        const auto wakeup = window->next_wakeup_ms();
        if (wakeup >= 0 && (timeout_ms < 0 || wakeup < timeout_ms))
        {
            timeout_ms = wakeup;
        }
    }
    const auto timeout = timeout_ms < 0 ? INFINITE : static_cast<DWORD>(timeout_ms);
    if (MsgWaitForMultipleObjectsEx(0, nullptr, timeout, QS_ALLINPUT, MWMO_INPUTAVAILABLE) == WAIT_FAILED)
    {
        throw std::runtime_error("等待 GUI 系统事件失败");
    }
}

void windows_window::show()
{
    ShowWindow(hwnd, SW_SHOW);
    InvalidateRect(hwnd, nullptr, FALSE);
}

void windows_window::set_title(const std::string& title)
{
    const auto text = utf16_text(decode_utf8(title).scalars);
    if (!SetWindowTextW(hwnd, text.c_str()))
    {
        throw std::runtime_error("无法设置窗口标题");
    }
}

void windows_window::present(const pixel_buffer& image)
{
    if (!hwnd || IsIconic(hwnd) || !IsWindowVisible(hwnd))
    {
        return;
    }
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = image.width();
    info.bmiHeader.biHeight = -static_cast<LONG>(image.height());
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    const auto dc = GetDC(hwnd);
    if (!dc)
    {
        throw std::runtime_error("无法取得像素提交目标");
    }
    const auto result = SetDIBitsToDevice(dc, 0, 0, image.width(), image.height(), 0, 0, 0,
        image.height(), image.pixels().data(), &info, DIB_RGB_COLORS);
    ReleaseDC(hwnd, dc);
    if (!result)
    {
        throw std::runtime_error("提交 GUI 像素失败");
    }
}

void windows_window::capture_pointer(bool enabled)
{
    if (enabled)
    {
        SetCapture(hwnd);
        if (GetCapture() != hwnd)
        {
            throw std::runtime_error("无法捕获鼠标");
        }
    }
    else if (GetCapture() == hwnd)
    {
        releasing_capture_ = true;
        ReleaseCapture();
        releasing_capture_ = false;
    }
}

void windows_window::focus()
{
    SetFocus(hwnd);
}

void windows_window::enqueue(window_event event)
{
    if (!events_.empty() && event.kind == event_kind::pointer_moved && events_.back().kind == event.kind)
    {
        events_.back() = std::move(event);
        return;
    }
    if (events_.size() >= 4096)
    {
        queue_failed_ = true;
        return;
    }
    events_.push_back(std::move(event));
}

std::optional<window_event> windows_window::next_event(int timeout_ms)
{
    if (timeout_ms < -1)
    {
        throw std::invalid_argument("事件等待时间非法");
    }
    const auto begin = std::chrono::steady_clock::now();
    while (hwnd)
    {
        MSG message{};
        unsigned count = 0;
        while (count++ < 64 && PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        if (queue_failed_)
        {
            queue_failed_ = false;
            throw std::runtime_error("系统事件队列溢出");
        }
        if (!events_.empty())
        {
            auto result = std::move(events_.front());
            events_.pop_front();
            return result;
        }
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - begin).count();
        if (timeout_ms >= 0 && elapsed >= timeout_ms)
        {
            return {};
        }
        const auto wait = timeout_ms < 0 ? INFINITE : static_cast<DWORD>(timeout_ms - elapsed);
        if (MsgWaitForMultipleObjectsEx(0, nullptr, wait, QS_ALLINPUT, MWMO_INPUTAVAILABLE) == WAIT_FAILED)
        {
            throw std::runtime_error("等待系统事件失败");
        }
    }
    return {};
}

LRESULT windows_window::message(UINT message, WPARAM wparam, LPARAM lparam)
{
    LRESULT result = 0;
    if (input(message, wparam, lparam, result))
    {
        return result;
    }
    switch (message)
    {
    case WM_CLOSE:
        enqueue({event_kind::close_requested});
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
    {
        PAINTSTRUCT paint{};
        BeginPaint(hwnd, &paint);
        EndPaint(hwnd, &paint);
        enqueue({event_kind::redraw});
        return 0;
    }
    case WM_SIZE:
    {
        window_event event{event_kind::resized};
        event.width = LOWORD(lparam);
        event.height = HIWORD(lparam);
        enqueue(std::move(event));
        return 0;
    }
    case WM_DPICHANGED:
    {
        scale_ = HIWORD(wparam) / 96.0;
        const auto& rect = *reinterpret_cast<RECT*>(lparam);
        SetWindowPos(hwnd, nullptr, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        return 0;
    }
    case WM_NCDESTROY:
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        hwnd = nullptr;
        return 0;
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

std::unique_ptr<platform_window> create_platform_window(const std::string& title, unsigned width, unsigned height)
{
    return std::make_unique<windows_window>(title, width, height);
}
}
