#define WINVER 0x0A00
#define _WIN32_WINNT 0x0A00
#include "stdlib/graphics/windows/state.hpp"
#include "backend/cpp/runtime_context.hpp"

#include <cmath>

namespace tx_generated::graphics
{
namespace
{

void initialize_dpi()
{
    if (AreDpiAwarenessContextsEqual(GetThreadDpiAwarenessContext(),
        DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
    {
        return;
    }
    if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) &&
        !AreDpiAwarenessContextsEqual(GetThreadDpiAwarenessContext(),
            DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
    {
        platform_fail(nullptr, "启用 Per-Monitor DPI V2", GetLastError());
    }
}

void register_window_class()
{
    WNDCLASSEXW info{};
    info.cbSize = sizeof(info);
    info.lpfnWndProc = window_proc;
    info.hInstance = GetModuleHandleW(nullptr);
    info.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    info.lpszClassName = L"tx_graphics_window_g1";
    if (!RegisterClassExW(&info) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        platform_fail(nullptr, "登记图形窗口类", GetLastError());
    }
}

UINT pixel_size(double dip, UINT dpi)
{
    const auto pixels = std::round(dip * dpi / 96.0);
    if (!std::isfinite(dip) || dip <= 0 || pixels < 1 || pixels > pixel_limit)
    {
        fail("invalid_argument", "客户区尺寸必须为正且每边不超过 16384 物理像素");
    }
    return static_cast<UINT>(pixels);
}

} // namespace

std::shared_ptr<app> open_app()
{
    auto& context = detail::current_runtime_context();
    if (!context.main_thread)
    {
        fail("wrong_thread", "图形会话必须在 TX 主线程初始化");
    }
    if (context.graphics_session)
    {
        auto& previous = *static_cast<app*>(context.graphics_session.get());
        drain(previous);
        if (previous.open)
        {
            fail("wrong_owner", "进程中已有活动图形会话");
        }
        context.close_graphics();
    }
    initialize_dpi();
    auto state = std::shared_ptr<app>(new app, [](app* value)
    {
        close_app(*value);
        delete value;
    });
    const auto initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(initialized))
    {
        platform_fail(state.get(), "初始化 STA COM", initialized);
    }
    state->com_initialized = true;
    ID2D1Factory* factory = nullptr;
    const auto created = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory);
    if (FAILED(created))
    {
        record_native_error(state.get(), created);
        fail("backend_unavailable", "创建 Direct2D 工厂失败，原生错误码：" + std::to_string(created));
    }
    state->factory.reset(factory);
    register_window_class();
    context.graphics_cleanup = [](void* value) noexcept
    {
        close_app(*static_cast<app*>(value));
    };
    context.graphics_session = state;
    return state;
}

std::wstring title_text(const std::string& value)
{
    if (value.size() > 65536 || value.find('\0') != std::string::npos)
    {
        fail("invalid_argument", "窗口标题不能包含 NUL 或超过 65536 字节");
    }
    if (value.empty())
    {
        return {};
    }
    const auto count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
        value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (!count)
    {
        fail("invalid_argument", "窗口标题不是有效 UTF-8");
    }
    std::wstring result(count, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
        static_cast<int>(value.size()), result.data(), count);
    return result;
}

std::shared_ptr<window> create_window(app& state, const std::string& title,
    double width, double height, bool resizable)
{
    if (state.frame)
    {
        fail("invalid_frame", "活动帧内不能创建窗口");
    }
    if (state.windows.size() >= window_limit)
    {
        fail("resource_limit", "同时存活窗口不能超过 64 个");
    }
    const auto text = title_text(title);
    auto child = std::make_shared<window>();
    child->owner = state.shared_from_this();
    child->id = state.next_id++;
    child->dpi = GetDpiForSystem();
    const auto style = resizable ? WS_OVERLAPPEDWINDOW :
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT bounds{0, 0, static_cast<LONG>(pixel_size(width, child->dpi)),
        static_cast<LONG>(pixel_size(height, child->dpi))};
    if (!AdjustWindowRectExForDpi(&bounds, style, FALSE, 0, child->dpi))
    {
        platform_fail(&state, "计算窗口客户区", GetLastError());
    }
    // 先登记控制块，WndProc 才能在 CreateWindowExW 的同步消息中安全引用。
    state.windows.push_back(child);
    const auto hwnd = CreateWindowExW(0, L"tx_graphics_window_g1", text.c_str(), style,
        CW_USEDEFAULT, CW_USEDEFAULT, bounds.right - bounds.left, bounds.bottom - bounds.top,
        nullptr, nullptr, GetModuleHandleW(nullptr), child.get());
    if (!hwnd)
    {
        const auto code = GetLastError();
        child->closed = true;
        state.windows.pop_back();
        platform_fail(&state, "创建窗口", code);
    }
    child->dpi = GetDpiForWindow(hwnd);
    // 系统默认放置的显示器可能与主显示器 DPI 不同，保持请求的 DIP 客户区。
    bounds = {0, 0, static_cast<LONG>(pixel_size(width, child->dpi)),
        static_cast<LONG>(pixel_size(height, child->dpi))};
    AdjustWindowRectExForDpi(&bounds, style, FALSE, 0, child->dpi);
    if (!SetWindowPos(hwnd, nullptr, 0, 0, bounds.right - bounds.left,
        bounds.bottom - bounds.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE))
    {
        const auto code = GetLastError();
        close_window(*child);
        platform_fail(&state, "设置客户区尺寸", code);
    }
    update_size(*child);
    return child;
}

void show_window(window& state)
{
    const auto owner = state.owner.lock();
    if (owner->frame)
    {
        fail("invalid_frame", "活动帧内不能展示窗口");
    }
    ShowWindow(state.hwnd, SW_SHOW);
    state.visible = true;
    invalidate(state);
}

void set_title(window& state, const std::string& title)
{
    const auto text = title_text(title);
    if (!SetWindowTextW(state.hwnd, text.c_str()))
    {
        platform_fail(state.owner.lock().get(), "修改窗口标题", GetLastError());
    }
}

void invalidate(window& state)
{
    if (!InvalidateRect(state.hwnd, nullptr, FALSE))
    {
        platform_fail(state.owner.lock().get(), "请求窗口重绘", GetLastError());
    }
}

} // namespace tx_generated::graphics
