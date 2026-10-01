#include "stdlib/graphics/windows/state.hpp"
#include "backend/cpp/runtime_context.hpp"
#include "stdlib/gui/windows/state.hpp"
#include "stdlib/graphics/windows/assets.hpp"
#include "stdlib/gui/windows/commands.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace tx_generated::graphics
{

[[noreturn]] void fail(const char* code, const std::string& message)
{
    throw runtime_failure({tx::error_kind::graphics, code, message});
}

void record_native_error(app* state, std::int64_t code) noexcept
{
    detail::current_runtime_context().graphics_native_error = code;
    if (state)
    {
        state->native_error = code;
    }
}

[[noreturn]] void platform_fail(app* state, const char* operation, std::int64_t code)
{
    record_native_error(state, code);
    fail("platform_error", std::string(operation) + "失败，原生错误码：" + std::to_string(code));
}

void require_thread(DWORD thread)
{
    if (thread != GetCurrentThreadId())
    {
        fail("wrong_thread", "图形资源只能在所属 UI 线程使用");
    }
}

app& require_app(resource* value)
{
    auto& state = *static_cast<app*>(value);
    require_thread(state.thread);
    drain(state);
    if (!state.open)
    {
        fail("closed_resource", "图形会话已经关闭");
    }
    return state;
}

window& require_window(resource* value, bool allow_closed)
{
    auto& state = *static_cast<window*>(value);
    require_thread(state.thread);
    if (const auto owner = state.owner.lock())
    {
        drain(*owner);
    }
    if (!allow_closed && (state.closed || !state.hwnd))
    {
        fail("closed_resource", "图形窗口已经关闭");
    }
    return state;
}

canvas& require_canvas(resource* value)
{
    auto& state = *static_cast<canvas*>(value);
    require_thread(state.thread);
    const auto owner = state.owner.lock();
    if (owner)
    {
        drain(*owner);
    }
    if (!owner || !owner->open || (state.target_window && state.target_window->closed) ||
        (state.target_surface && state.target_surface->closed))
    {
        fail("closed_resource", "画布所属的会话或窗口已经关闭");
    }
    if (!state.active || owner->frame.get() != &state)
    {
        fail("invalid_frame", "画布已经提交、取消或不属于当前帧");
    }
    return state;
}

void cancel_canvas(canvas& state) noexcept
{
    if (state.active && state.target)
    {
        pop_clips(state, 0);
        state.target->EndDraw();
    }
    state.active = false;
    state.brush.reset();
    state.target.reset();
    state.bitmap_target.reset();
    state.saved.clear();
    if (state.target_surface)
    {
        state.target_surface->pending.reset();
    }
}

void close_window(window& state) noexcept
{
    if (state.closed)
    {
        return;
    }
    if (const auto owner = state.owner.lock(); owner && owner->frame &&
        owner->frame->target_window.get() == &state)
    {
        cancel_canvas(*owner->frame);
        owner->frame.reset();
    }
    state.target.reset();
    reset_input(state);
    gui::close_root(state);
    gui::close_interactions(state);
    if (state.hwnd)
    {
        DestroyWindow(state.hwnd);
    }
    state.closed = true;
    state.hwnd = nullptr;
}

void close_app(app& state) noexcept
{
    if (!state.open)
    {
        return;
    }
    if (state.frame)
    {
        cancel_canvas(*state.frame);
        state.frame.reset();
    }
    for (const auto& child : state.windows)
    {
        close_window(*child);
    }
    state.windows.clear();
    for (const auto& resource : state.resources)
    {
        close_owned(*resource);
    }
    state.resources.clear();
    state.events.clear();
    state.factory.reset();
    if (state.com_initialized)
    {
        CoUninitialize();
        state.com_initialized = false;
    }
    state.open = false;
}

void drain(app& state)
{
    if (state.draining || !state.open)
    {
        return;
    }
    state.draining = true;
    if (state.abandoned.load(std::memory_order_acquire))
    {
        close_app(state);
    }
    else
    {
        if (state.frame && state.frame->abandoned.load(std::memory_order_acquire))
        {
            cancel_canvas(*state.frame);
            state.frame.reset();
        }
        for (const auto& child : state.windows)
        {
            if (child->abandoned.load(std::memory_order_acquire))
            {
                close_window(*child);
            }
        }
        std::erase_if(state.windows, [](const auto& child)
        {
            return child->closed;
        });
        collect_resources(state);
    }
    state.draining = false;
}

float checked_number(double value)
{
    if (!std::isfinite(value) || std::abs(value) > std::numeric_limits<float>::max())
    {
        fail("invalid_argument", "图形坐标必须为后端可表示的有限数");
    }
    return static_cast<float>(value);
}

float checked_width(double value)
{
    const auto result = checked_number(value);
    if (result <= 0)
    {
        fail("invalid_argument", "线宽必须大于零");
    }
    return result;
}

D2D1_COLOR_F checked_color(rgba value)
{
    for (const auto channel : {value.red, value.green, value.blue, value.alpha})
    {
        if (!std::isfinite(channel) || channel < 0 || channel > 1)
        {
            fail("invalid_argument", "颜色通道必须位于 [0, 1]");
        }
    }
    return D2D1::ColorF(static_cast<float>(value.red), static_cast<float>(value.green),
        static_cast<float>(value.blue), static_cast<float>(value.alpha));
}

D2D1_RECT_F checked_rect(rectangle value)
{
    checked_number(value.width);
    checked_number(value.height);
    if (value.width < 0 || value.height < 0)
    {
        fail("invalid_argument", "矩形宽高不能为负");
    }
    return D2D1::RectF(checked_number(value.x), checked_number(value.y),
        checked_number(value.x + value.width), checked_number(value.y + value.height));
}

} // namespace tx_generated::graphics
