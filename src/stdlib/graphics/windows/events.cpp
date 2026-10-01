#include "stdlib/graphics/windows/state.hpp"

#include <algorithm>
#include <limits>

namespace tx_generated::graphics
{

void enqueue(window& state, const char* kind, bool with_size) noexcept
{
    const auto owner = state.owner.lock();
    if (!owner || !owner->open)
    {
        return;
    }
    try
    {
        if (std::string_view(kind) == "paint" && std::any_of(owner->events.begin(),
            owner->events.end(), [&](const event& item)
            {
                return item.kind == "paint" && item.window_id == state.id;
            }))
        {
            return;
        }
        if (owner->events.size() >= event_limit)
        {
            const auto removable = std::find_if(owner->events.begin(), owner->events.end(),
                [](const event& item)
                {
                    return item.kind == "paint" || item.kind == "resized" || item.kind == "dpi_changed";
                });
            if (removable == owner->events.end())
            {
                owner->queue_failed = true;
                ReleaseCapture();
                return;
            }
            owner->events.erase(removable);
        }
        event item{kind, state.id, std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - owner->start).count(), {}};
        if (with_size)
        {
            item.resize = size_event{state.width * 96.0 / state.dpi,
                state.height * 96.0 / state.dpi, state.width, state.height, state.dpi};
        }
        owner->events.push_back(std::move(item));
    }
    catch (...)
    {
        owner->queue_failed = true;
    }
}

void update_size(window& state)
{
    RECT bounds{};
    if (!GetClientRect(state.hwnd, &bounds))
    {
        platform_fail(state.owner.lock().get(), "读取客户区", GetLastError());
    }
    state.width = static_cast<UINT>(std::max<LONG>(0, bounds.right));
    state.height = static_cast<UINT>(std::max<LONG>(0, bounds.bottom));
    if (state.target && state.width && state.height)
    {
        state.target->SetDpi(static_cast<float>(state.dpi), static_cast<float>(state.dpi));
        const auto status = state.target->Resize(D2D1::SizeU(state.width, state.height));
        if (FAILED(status))
        {
            record_native_error(state.owner.lock().get(), status);
            state.target.reset();
        }
    }
}

LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) noexcept
{
    auto* state = reinterpret_cast<window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE)
    {
        state = static_cast<window*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);
        state->hwnd = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    }
    if (!state)
    {
        return DefWindowProcW(hwnd, message, wparam, lparam);
    }
    try
    {
        switch (message)
        {
        case WM_CLOSE:
            enqueue(*state, "close_requested");
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT:
        {
            PAINTSTRUCT paint{};
            BeginPaint(hwnd, &paint);
            EndPaint(hwnd, &paint);
            enqueue(*state, "paint");
            return 0;
        }
        case WM_SIZE:
            state->minimized = wparam == SIZE_MINIMIZED;
            update_size(*state);
            if (!state->changing_dpi)
            {
                enqueue(*state, "resized", true);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        case WM_DPICHANGED:
        {
            state->dpi = HIWORD(wparam);
            const auto& bounds = *reinterpret_cast<RECT*>(lparam);
            state->changing_dpi = true;
            const auto moved = SetWindowPos(hwnd, nullptr, bounds.left, bounds.top,
                bounds.right - bounds.left, bounds.bottom - bounds.top, SWP_NOZORDER | SWP_NOACTIVATE);
            state->changing_dpi = false;
            if (!moved)
            {
                platform_fail(state->owner.lock().get(), "应用 DPI 建议矩形", GetLastError());
            }
            update_size(*state);
            enqueue(*state, "dpi_changed", true);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        case WM_NCDESTROY:
            state->hwnd = nullptr;
            state->closed = true;
            enqueue(*state, "closed");
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            break;
        }
    }
    catch (...)
    {
        if (const auto owner = state->owner.lock())
        {
            owner->queue_failed = true;
        }
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

std::optional<event> next_event(app& state, std::int64_t timeout)
{
    if (timeout < -1)
    {
        fail("invalid_argument", "事件等待时间必须为 -1、0 或正整数");
    }
    if (state.frame)
    {
        fail("invalid_frame", "活动帧内不能取事件");
    }
    const auto started = std::chrono::steady_clock::now();
    unsigned processed = 0;
    while (true)
    {
        MSG message{};
        while (processed < 64 && PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
        {
            ++processed;
            if (message.message == WM_QUIT)
            {
                close_app(state);
                return {};
            }
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        if (state.queue_failed)
        {
            state.queue_failed = false;
            fail("resource_limit", "窗口消息处理失败或事件队列已满");
        }
        if (!state.events.empty())
        {
            auto result = std::move(state.events.front());
            state.events.pop_front();
            return result;
        }
        drain(state);
        if (!state.open || state.windows.empty() || timeout == 0 || processed == 64)
        {
            return {};
        }
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started).count();
        if (timeout >= 0 && elapsed >= timeout)
        {
            return {};
        }
        const DWORD wait = timeout < 0 ? INFINITE : static_cast<DWORD>(
            std::min<std::int64_t>(timeout - elapsed, INFINITE - 1));
        const auto status = MsgWaitForMultipleObjectsEx(0, nullptr, wait,
            QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        if (status == WAIT_FAILED)
        {
            platform_fail(&state, "等待图形事件", GetLastError());
        }
    }
}

} // namespace tx_generated::graphics
