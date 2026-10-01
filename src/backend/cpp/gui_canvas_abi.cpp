#include "backend/cpp/gui_complex_abi.hpp"
#include "backend/cpp/graphics_result.hpp"
#include "stdlib/gui/windows/complex.hpp"

using namespace tx_generated;
using namespace tx_generated::gui;

extern "C" int txrt_gui_create_canvas(tx_generated::graphics::resource* parent, const char* type, void** result) noexcept
{
    return graphics::result_call(type, result, [&]() -> std::any
    {
        auto state = create(require_node(parent), tx::graphics_kind::gui_canvas);
        sync_canvas(*state);
        return graphics::make_handle(state);
    });
}

extern "C" int txrt_gui_begin_canvas(tx_generated::graphics::resource* control, const char* type, void** result) noexcept
{
    return graphics::result_call(type, result, [&]() -> std::any
    {
        const auto frame = begin_canvas(require_node(control));
        return graphics::option_value("option<graphics_canvas>", frame ? std::any(graphics::make_handle(frame)) : std::any{});
    });
}

extern "C" int txrt_gui_invalidate_canvas(tx_generated::graphics::resource* control) noexcept
{
    return detail::invoke_leaf([&]
    {
        InvalidateRect(require_node(control).hwnd, nullptr, FALSE);
    });
}

extern "C" int txrt_gui_canvas_width(tx_generated::graphics::resource* control, double* result) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_node(control);
        gui::flush_layout(owner_window(state));
        sync_canvas(state);
        *result = state.canvas_window->width * 96.0 / state.canvas_window->dpi;
    });
}

extern "C" int txrt_gui_canvas_height(tx_generated::graphics::resource* control, double* result) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_node(control);
        gui::flush_layout(owner_window(state));
        sync_canvas(state);
        *result = state.canvas_window->height * 96.0 / state.canvas_window->dpi;
    });
}

extern "C" int txrt_gui_high_contrast(bool* result) noexcept
{
    return detail::invoke_leaf([&]
    {
        HIGHCONTRASTW settings{};
        settings.cbSize = sizeof(settings);
        if (!SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(settings), &settings, 0))
        {
            platform_fail(nullptr, "读取高对比度状态", GetLastError());
        }
        *result = (settings.dwFlags & HCF_HIGHCONTRASTON) != 0;
    });
}

extern "C" int txrt_gui_theme_color(const void* role, const tx_generated::record_type* type, void** result) noexcept
{
    return detail::invoke_leaf([&]
    {
        const auto& name = detail::text_value(role);
        int index = 0;
        if (name == "background")
        {
            index = COLOR_BTNFACE;
        }
        else if (name == "text")
        {
            index = COLOR_BTNTEXT;
        }
        else if (name == "window")
        {
            index = COLOR_WINDOW;
        }
        else if (name == "window_text")
        {
            index = COLOR_WINDOWTEXT;
        }
        else if (name == "highlight")
        {
            index = COLOR_HIGHLIGHT;
        }
        else if (name == "highlight_text")
        {
            index = COLOR_HIGHLIGHTTEXT;
        }
        else
        {
            fail("invalid_argument", "未知的系统语义颜色");
        }
        const auto color = GetSysColor(index);
        dynamic_struct value{dynamic_struct_data(type)};
        value->write_field(0, GetRValue(color) / 255.0);
        value->write_field(1, GetGValue(color) / 255.0);
        value->write_field(2, GetBValue(color) / 255.0);
        value->write_field(3, 1.0);
        *result = detail::make_handle<std::any>(std::move(value));
    });
}
