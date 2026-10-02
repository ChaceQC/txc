#include "backend/cpp/native_gui_result.hpp"
#include "backend/cpp/native_gui_abi.hpp"
#include "backend/cpp/native_gui_extended_abi.hpp"
#include "stdlib/native_gui/containers.hpp"

using namespace tx_generated;
using graphics::resource;

extern "C" int txrt_native_gui_create_scroll(resource* parent, bool horizontal, bool vertical,
    const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        return graphics::make_handle(native_gui::create_scroll(native_gui::require_node(parent), horizontal, vertical));
    });
}

extern "C" int txrt_native_gui_scroll_content(resource* control, const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        return graphics::make_handle(native_gui::content_panel(native_gui::require_node(control), 0));
    });
}

extern "C" int txrt_native_gui_scroll_to(resource* control, double x, double y) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& state = native_gui::require_node(control);
        native_gui::layout(state);
        native_gui::scroll_to(state, x, y, false);
    });
}

extern "C" int txrt_native_gui_scroll_x(resource* control, double* result) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& state = native_gui::require_node(control);
        native_gui::layout(state);
        *result = state.scroll->x;
    });
}

extern "C" int txrt_native_gui_scroll_y(resource* control, double* result) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& state = native_gui::require_node(control);
        native_gui::layout(state);
        *result = state.scroll->y;
    });
}

extern "C" int txrt_native_gui_create_tabs(resource* parent, const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        return graphics::make_handle(native_gui::create_tabs(native_gui::require_node(parent)));
    });
}

extern "C" int txrt_native_gui_add_tab(resource* control, const void* title, const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        return graphics::make_handle(native_gui::add_tab(native_gui::require_node(control), detail::text_value(title)));
    });
}

extern "C" int txrt_native_gui_select_tab(resource* control, std::int64_t id) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::select_tab(native_gui::require_node(control), id, false);
    });
}

extern "C" int txrt_native_gui_selected_tab(resource* control, std::int64_t* result) noexcept
{
    return native_gui::leaf_call([&]
    {
        *result = native_gui::require_node(control).tabs->selected;
    });
}

extern "C" int txrt_native_gui_tab_count(resource* control, std::int64_t* result) noexcept
{
    return native_gui::leaf_call([&]
    {
        *result = native_gui::require_node(control).children.size();
    });
}

extern "C" int txrt_native_gui_set_tab_title(resource* control, std::int64_t id, const void* title) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& state = native_gui::require_node(control);
        for (const auto& page : state.children)
        {
            if (page->id == id)
            {
                native_gui::set_text(*page, detail::text_value(title));
                return;
            }
        }
        native_gui::fail("invalid_argument", "页 ID 不存在");
    });
}

extern "C" int txrt_native_gui_create_split(resource* parent, bool horizontal, double ratio,
    const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        return graphics::make_handle(native_gui::create_split(native_gui::require_node(parent), horizontal, ratio));
    });
}

extern "C" int txrt_native_gui_split_first(resource* control, const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        return graphics::make_handle(native_gui::content_panel(native_gui::require_node(control), 0));
    });
}

extern "C" int txrt_native_gui_split_second(resource* control, const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        return graphics::make_handle(native_gui::content_panel(native_gui::require_node(control), 1));
    });
}

extern "C" int txrt_native_gui_set_split_ratio(resource* control, double ratio) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::set_split_ratio(native_gui::require_node(control), ratio, false);
    });
}

extern "C" int txrt_native_gui_split_ratio(resource* control, double* result) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& state = native_gui::require_node(control);
        native_gui::layout(state);
        *result = state.split->ratio;
    });
}
