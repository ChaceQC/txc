#include "backend/cpp/graphics_result.hpp"
#include "stdlib/graphics/windows/assets.hpp"

using namespace tx_generated;
using namespace tx_generated::graphics;

extern "C" int txrt_graphics_close_image(resource* value) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = *static_cast<image*>(value);
        require_thread(state.thread);
        close_owned(state);
    });
}

extern "C" int txrt_graphics_close_surface(resource* value) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = *static_cast<surface*>(value);
        require_thread(state.thread);
        close_owned(state);
    });
}

extern "C" int txrt_graphics_close_path_resource(resource* value) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = *static_cast<path*>(value);
        require_thread(state.thread);
        close_owned(state);
    });
}

extern "C" int txrt_graphics_close_brush(resource* value) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = *static_cast<brush*>(value);
        require_thread(state.thread);
        close_owned(state);
    });
}

extern "C" int txrt_graphics_close_font(resource* value) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = *static_cast<font*>(value);
        require_thread(state.thread);
        close_owned(state);
    });
}

extern "C" int txrt_graphics_close_text_layout(resource* value) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = *static_cast<text_layout*>(value);
        require_thread(state.thread);
        close_owned(state);
    });
}
