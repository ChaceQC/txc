#include "backend/cpp/graphics_abi.hpp"
#include "backend/cpp/graphics_result.hpp"
#include "stdlib/graphics/windows/state.hpp"

using namespace tx_generated;
using namespace tx_generated::graphics;

extern "C" int txrt_graphics_open_app(const char* type, void** result) noexcept
{
    return result_call(type, result, []() -> std::any
    {
        return make_handle(open_app());
    });
}

extern "C" int txrt_graphics_close_app(resource* value) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = *static_cast<app*>(value);
        require_thread(state.thread);
        close_app(state);
    });
}

extern "C" int txrt_graphics_close_window(resource* value) noexcept
{
    return detail::invoke_leaf([&]
    {
        close_window(require_window(value, true));
    });
}

extern "C" int txrt_graphics_create_window(resource* value, const void* title,
    double width, double height, bool resizable, const char* type, void** result) noexcept
{
    return result_call(type, result, [&]() -> std::any
    {
        return make_handle(create_window(require_app(value), detail::text_value(title),
            width, height, resizable));
    });
}

extern "C" int txrt_graphics_is_open(resource* value, bool* result) noexcept
{
    return detail::invoke_leaf([&]
    {
        *result = !require_window(value, true).closed;
    });
}

extern "C" int txrt_graphics_show(resource* value, const char* type, void** result) noexcept
{
    return result_call(type, result, [&]() -> std::any
    {
        show_window(require_window(value));
        return {};
    });
}

extern "C" int txrt_graphics_set_title(resource* value, const void* title,
    const char* type, void** result) noexcept
{
    return result_call(type, result, [&]() -> std::any
    {
        set_title(require_window(value), detail::text_value(title));
        return {};
    });
}

extern "C" int txrt_graphics_window_id(resource* value, std::int64_t* result) noexcept
{
    return detail::invoke_leaf([&]
    {
        *result = require_window(value).id;
    });
}

extern "C" int txrt_graphics_invalidate(resource* value) noexcept
{
    return detail::invoke_leaf([&]
    {
        invalidate(require_window(value));
    });
}

extern "C" int txrt_graphics_backend(resource* value, void** result) noexcept
{
    return detail::invoke_leaf([&]
    {
        const auto& state = require_window(value);
        *result = detail::make_handle<std::string>(!state.target ? "uninitialized" :
            state.software ? "software" : "hardware");
    });
}

extern "C" int txrt_graphics_last_native_error(std::int64_t* result) noexcept
{
    return detail::invoke_leaf([&]
    {
        *result = detail::current_runtime_context().graphics_native_error;
    });
}

extern "C" int txrt_graphics_begin_frame(resource* value, const char* type, void** result) noexcept
{
    return result_call(type, result, [&]() -> std::any
    {
        const auto frame = begin_frame(require_window(value));
        return option_value("option<graphics_canvas>", frame ? std::any(make_handle(frame)) : std::any{});
    });
}

extern "C" int txrt_graphics_end_frame(resource* value, const char* type, void** result) noexcept
{
    return result_call(type, result, [&]() -> std::any
    {
        end_frame(require_canvas(value));
        return {};
    });
}

extern "C" int txrt_graphics_cancel_frame(resource* value) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_canvas(value);
        cancel_canvas(state);
        state.owner.lock()->frame.reset();
    });
}
