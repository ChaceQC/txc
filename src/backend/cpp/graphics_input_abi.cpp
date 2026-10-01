#include "backend/cpp/graphics_result.hpp"
#include "stdlib/graphics/windows/assets.hpp"
#include <imm.h>
#include <algorithm>
#include <cmath>

using namespace tx_generated;
using namespace tx_generated::graphics;

extern "C" int txrt_graphics_capture_pointer(resource* value, const char* type, void** result) noexcept
{
    return result_call(type, result, [&]() -> std::any
    {
        auto& state = require_window(value);
        SetCapture(state.hwnd);
        if (GetCapture() != state.hwnd)
        {
            platform_fail(state.owner.lock().get(), "捕获鼠标", GetLastError());
        }
        return {};
    });
}

extern "C" int txrt_graphics_release_pointer(resource* value) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_window(value);
        if (GetCapture() == state.hwnd && !ReleaseCapture())
        {
            platform_fail(state.owner.lock().get(), "释放鼠标捕获", GetLastError());
        }
    });
}

extern "C" int txrt_graphics_key_pressed(resource* value, const void* key, bool* result) noexcept
{
    return detail::invoke_leaf([&]
    {
        *result = require_window(value).keys[key_code(detail::text_value(key))];
    });
}

extern "C" int txrt_graphics_enable_text_input(resource* value, bool enabled) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_window(value);
        if (!enabled)
        {
            const auto context = ImmGetContext(state.hwnd);
            if (context)
            {
                ImmNotifyIME(context, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
                ImmReleaseContext(state.hwnd, context);
            }
            reset_input(state);
        }
        state.text_input = enabled;
        ImmAssociateContextEx(state.hwnd, nullptr, enabled ? IACE_DEFAULT : 0);
        if (enabled)
        {
            position_ime(state);
        }
    });
}

extern "C" int txrt_graphics_set_ime_rect(resource* value, double x, double y, double width, double height) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_window(value);
        const auto rect = checked_rect({x, y, width, height});
        for (const auto coordinate : {rect.left, rect.top, rect.right, rect.bottom})
        {
            if (std::abs(coordinate) > 1000000)
            {
                fail("invalid_argument", "输入法插入点坐标超限");
            }
        }
        state.ime_rect = {static_cast<LONG>(std::floor(rect.left)), static_cast<LONG>(std::floor(rect.top)),
            static_cast<LONG>(std::ceil(rect.right)), static_cast<LONG>(std::ceil(rect.bottom))};
        position_ime(state);
    });
}

extern "C" int txrt_graphics_start_timer(resource* value, std::int64_t interval, const char* type, void** result) noexcept
{
    return result_call(type, result, [&]() -> std::any
    {
        auto& state = require_window(value);
        const auto owner = state.owner.lock();
        if (interval < 10 || interval > 2147483647)
        {
            fail("invalid_argument", "定时器间隔必须为 10–2147483647 毫秒");
        }
        if (state.timers.size() >= 256)
        {
            fail("resource_limit", "每窗口最多 256 个定时器");
        }
        const auto id = static_cast<UINT_PTR>(owner->next_id++);
        state.timers.emplace(id, static_cast<UINT>(interval));
        if (!state.minimized && !SetTimer(state.hwnd, id, static_cast<UINT>(interval), nullptr))
        {
            state.timers.erase(id);
            platform_fail(owner.get(), "启动定时器", GetLastError());
        }
        return static_cast<std::int64_t>(id);
    });
}

extern "C" int txrt_graphics_stop_timer(resource* value, std::int64_t id) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_window(value);
        if (!state.timers.contains(static_cast<UINT_PTR>(id)))
        {
            fail("invalid_argument", "定时器不属于此窗口或已经停止");
        }
        KillTimer(state.hwnd, static_cast<UINT_PTR>(id));
        state.timers.erase(static_cast<UINT_PTR>(id));
        std::erase_if(state.owner.lock()->events, [&](const event& item)
        {
            return item.timer_id == id;
        });
    });
}
