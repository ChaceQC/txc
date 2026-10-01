#include "backend/cpp/gui_complex_abi.hpp"
#include "backend/cpp/graphics_result.hpp"
#include "stdlib/gui/windows/complex.hpp"

using namespace tx_generated;
using namespace tx_generated::gui;

extern "C" int txrt_gui_create_tabs(tx_generated::graphics::resource* parent, const char* type, void** result) noexcept
{
    return graphics::result_call(type, result, [&]() -> std::any
    {
        return graphics::make_handle(create(require_node(parent), tx::graphics_kind::tabs));
    });
}

extern "C" int txrt_gui_add_tab(tx_generated::graphics::resource* tabs, const void* title, const char* type, void** result) noexcept
{
    return graphics::result_call(type, result, [&]() -> std::any
    {
        return graphics::make_handle(add_tab(require_node(tabs), detail::text_value(title)));
    });
}

extern "C" int txrt_gui_select_tab(tx_generated::graphics::resource* tabs, tx_generated::graphics::resource* page) noexcept
{
    return detail::invoke_leaf([&]
    {
        select_tab(require_node(tabs), require_node(page));
    });
}

extern "C" int txrt_gui_selected_tab(tx_generated::graphics::resource* tabs, const char* type, void** result) noexcept
{
    return detail::invoke_leaf([&]
    {
        const auto page = require_node(tabs).selected_page.lock();
        *result = detail::make_handle<std::any>(graphics::option_value(type, page ? std::any(page->id) : std::any{}));
    });
}

extern "C" int txrt_gui_create_scroll(tx_generated::graphics::resource* parent, bool horizontal, bool vertical, const char* type, void** result) noexcept
{
    return graphics::result_call(type, result, [&]() -> std::any
    {
        return graphics::make_handle(create_scroll(require_node(parent), horizontal, vertical));
    });
}

extern "C" int txrt_gui_set_scroll_position(tx_generated::graphics::resource* control, double x, double y) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_node(control);
        require_idle(state);
        gui::flush_layout(owner_window(state));
        set_scroll_position(state, x, y);
    });
}

extern "C" int txrt_gui_scroll_x(tx_generated::graphics::resource* control, double* result) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_node(control);
        gui::flush_layout(owner_window(state));
        *result = state.scroll_x;
    });
}

extern "C" int txrt_gui_scroll_y(tx_generated::graphics::resource* control, double* result) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_node(control);
        gui::flush_layout(owner_window(state));
        *result = state.scroll_y;
    });
}

extern "C" int txrt_gui_create_split(tx_generated::graphics::resource* parent, bool vertical, double position, double minimum_first, double minimum_second, const char* type, void** result) noexcept
{
    return graphics::result_call(type, result, [&]() -> std::any
    {
        return graphics::make_handle(create_split(require_node(parent), vertical, position, minimum_first, minimum_second));
    });
}

extern "C" int txrt_gui_split_pane(tx_generated::graphics::resource* control, std::int64_t index, const char* type, void** result) noexcept
{
    return graphics::result_call(type, result, [&]() -> std::any
    {
        auto& state = require_node(control);
        if (!state.split || index < 0 || static_cast<std::size_t>(index) >= state.children.size())
        {
            fail("invalid_argument", "分栏页面索引必须为存在的 0/1 页面");
        }
        return graphics::make_handle(state.children[index]);
    });
}

extern "C" int txrt_gui_set_split_position(tx_generated::graphics::resource* control, double position) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_split_position(require_node(control), position);
    });
}

extern "C" int txrt_gui_split_position(tx_generated::graphics::resource* control, double* result) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_node(control);
        if (!state.split)
        {
            fail("invalid_argument", "控件不是分栏容器");
        }
        *result = state.split_position;
    });
}

extern "C" int txrt_gui_create_progress_bar(tx_generated::graphics::resource* parent, const char* type, void** result) noexcept
{
    return graphics::result_call(type, result, [&]() -> std::any
    {
        return graphics::make_handle(create(require_node(parent), tx::graphics_kind::progress_bar));
    });
}

extern "C" int txrt_gui_set_progress(tx_generated::graphics::resource* control, std::int64_t minimum, std::int64_t maximum, std::int64_t value) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_progress(require_node(control), minimum, maximum, value);
    });
}

extern "C" int txrt_gui_progress_value(tx_generated::graphics::resource* control, std::int64_t* result) noexcept
{
    return detail::invoke_leaf([&]
    {
        *result = require_node(control).range_value;
    });
}

extern "C" int txrt_gui_set_indeterminate(tx_generated::graphics::resource* control, bool enabled) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_node(control);
        auto style = GetWindowLongPtrW(state.hwnd, GWL_STYLE);
        SetWindowLongPtrW(state.hwnd, GWL_STYLE, enabled ? style | PBS_MARQUEE : style & ~PBS_MARQUEE);
        SendMessageW(state.hwnd, PBM_SETMARQUEE, enabled, 30);
        state.indeterminate = enabled;
        if (!enabled)
        {
            SendMessageW(state.hwnd, PBM_SETPOS, state.range_value, 0);
        }
        ++state.revision;
    });
}

extern "C" int txrt_gui_create_slider(tx_generated::graphics::resource* parent, std::int64_t minimum, std::int64_t maximum, std::int64_t value, std::int64_t step, const char* type, void** result) noexcept
{
    return graphics::result_call(type, result, [&]() -> std::any
    {
        if (minimum < 0 || maximum > INT_MAX || maximum <= minimum || value < minimum || value > maximum ||
            step < 1 || step > maximum - minimum)
        {
            fail("invalid_argument", "滑块范围、值或步长无效");
        }
        auto state = create(require_node(parent), tx::graphics_kind::slider);
        state->range_minimum = minimum;
        state->range_maximum = maximum;
        state->range_step = step;
        SendMessageW(state->hwnd, TBM_SETRANGEMIN, FALSE, minimum);
        SendMessageW(state->hwnd, TBM_SETRANGEMAX, FALSE, maximum);
        SendMessageW(state->hwnd, TBM_SETLINESIZE, 0, step);
        SendMessageW(state->hwnd, TBM_SETPAGESIZE, 0, step);
        set_slider_value(*state, value);
        return graphics::make_handle(state);
    });
}

extern "C" int txrt_gui_set_slider_value(tx_generated::graphics::resource* control, std::int64_t value) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_slider_value(require_node(control), value);
    });
}

extern "C" int txrt_gui_slider_value(tx_generated::graphics::resource* control, std::int64_t* result) noexcept
{
    return detail::invoke_leaf([&]
    {
        *result = require_node(control).range_value;
    });
}
