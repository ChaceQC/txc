#include "stdlib/graphics/windows/state.hpp"
#include "stdlib/gui/windows/state.hpp"
#include "stdlib/gui/windows/commands.hpp"

#include <algorithm>
#include <limits>

namespace tx_generated::graphics
{

void enqueue_event(window& state, event item)
{
    const auto owner = state.owner.lock();
    if (!owner || !owner->open)
    {
        return;
    }
    item.window_id = state.id;
    if (const auto source = state.gui_canvas_source.lock(); source && !item.control)
    {
        item.control = event::control_data{};
        item.control->source_id = source->id;
        item.control->action = item.kind;
        item.control->revision = source->revision;
    }
    item.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - owner->start).count();
    if (item.kind == "timer" && std::any_of(owner->events.begin(), owner->events.end(),
        [&](const event& queued)
        {
            return queued.timer_id == item.timer_id;
        }))
    {
        return;
    }
    if (item.kind == "pointer_moved" && !owner->events.empty() &&
        owner->events.back().kind == item.kind && owner->events.back().window_id == state.id &&
        (owner->events.back().control ? owner->events.back().control->source_id : 0) ==
            (item.control ? item.control->source_id : 0))
    {
        owner->events.back() = std::move(item);
        return;
    }
    if (owner->events.size() >= event_limit)
    {
        const auto removable = std::find_if(owner->events.begin(), owner->events.end(), [](const event& queued)
        {
            return queued.kind == "paint" || queued.kind == "pointer_moved" ||
                queued.kind == "resized" || queued.kind == "dpi_changed" || queued.kind == "timer";
        });
        if (removable == owner->events.end())
        {
            owner->queue_failed = true;
            for (const auto& window : owner->windows)
            {
                reset_input(*window);
                if (window->gui_root)
                {
                    gui::reset_interaction(*window->gui_root);
                }
            }
            return;
        }
        owner->events.erase(removable);
    }
    owner->events.push_back(std::move(item));
}

void enqueue_control(window& state, event::control_data data)
{
    const auto owner = state.owner.lock();
    if (!owner || !owner->open)
    {
        return;
    }
    event item;
    item.kind = "control";
    item.window_id = state.id;
    item.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - owner->start).count();
    item.control = std::move(data);
    // 只合并相邻编辑通知；提交、焦点和其他离散事件形成屏障。
    if (!owner->events.empty() && item.control->action == "text_changed")
    {
        auto& last = owner->events.back();
        if (last.control && last.control->source_id == item.control->source_id &&
            last.control->action == "text_changed")
        {
            last = std::move(item);
            return;
        }
    }
    enqueue_event(state, std::move(item));
}

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
        event item;
        item.kind = kind;
        if (with_size)
        {
            item.resize = size_event{state.width * 96.0 / state.dpi,
                state.height * 96.0 / state.dpi, state.width, state.height, state.dpi};
        }
        enqueue_event(state, std::move(item));
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
    gui::window_changed(state);
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
        LRESULT input_result = 0;
        if (input_message(*state, message, wparam, lparam, input_result))
        {
            return input_result;
        }
        switch (message)
        {
        case WM_ENABLE:
            if (state->gui_root)
            {
                gui::refresh_accessibility(*state->gui_root);
            }
            break;
        case WM_ACTIVATEAPP:
            if (!wparam && state->gui_root)
            {
                gui::reset_interaction(*state->gui_root);
            }
            break;
        case WM_COMMAND:
            if (!lparam && gui::activate_command(*state, LOWORD(wparam)))
            {
                return 0;
            }
            break;
        case WM_CLOSE:
            enqueue(*state, "close_requested");
            return 0;
        case WM_SETTINGCHANGE:
        case WM_THEMECHANGED:
        case WM_SYSCOLORCHANGE:
            gui::window_changed(*state, true);
            break;
        case WM_DESTROY:
            gui::close_root(*state);
            gui::close_interactions(*state);
            break;
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
            for (const auto& [id, interval] : state->timers)
            {
                if (state->minimized)
                {
                    KillTimer(hwnd, id);
                }
                else
                {
                    SetTimer(hwnd, id, interval, nullptr);
                }
            }
            if (state->minimized)
            {
                if (const auto owner = state->owner.lock())
                {
                    std::erase_if(owner->events, [&](const event& item)
                    {
                        return item.window_id == state->id && item.timer_id.has_value();
                    });
                }
            }
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
            position_ime(*state);
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
    catch (const runtime_failure& error)
    {
        if (const auto owner = state->owner.lock())
        {
            try
            {
                owner->pending_error = error.error();
            }
            catch (...)
            {
                owner->queue_failed = true;
            }
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
            if (!gui::translate_shortcut(state, message) && !gui::translate_message(state, message))
            {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
        }
        gui::flush_all(state);
        if (state.pending_error)
        {
            auto error = std::move(*state.pending_error);
            state.pending_error.reset();
            throw runtime_failure(std::move(error));
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
