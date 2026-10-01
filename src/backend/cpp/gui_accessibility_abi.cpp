#include "backend/cpp/gui_complex_abi.hpp"
#include "backend/cpp/graphics_result.hpp"
#include "stdlib/gui/windows/complex.hpp"

using namespace tx_generated;
using namespace tx_generated::gui;

extern "C" int txrt_gui_set_accessibility_container(graphics::resource* control, const void* name, const void* help) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_accessibility(require_node(control), detail::text_value(name), detail::text_value(help));
    });
}

extern "C" int txrt_gui_set_label_container(graphics::resource* control, graphics::resource* label) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_label(require_node(control), require_node(label));
    });
}

extern "C" int txrt_gui_set_accessibility_label(graphics::resource* control, const void* name, const void* help) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_accessibility(require_node(control), detail::text_value(name), detail::text_value(help));
    });
}

extern "C" int txrt_gui_set_label_label(graphics::resource* control, graphics::resource* label) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_label(require_node(control), require_node(label));
    });
}

extern "C" int txrt_gui_set_accessibility_button(graphics::resource* control, const void* name, const void* help) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_accessibility(require_node(control), detail::text_value(name), detail::text_value(help));
    });
}

extern "C" int txrt_gui_set_label_button(graphics::resource* control, graphics::resource* label) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_label(require_node(control), require_node(label));
    });
}

extern "C" int txrt_gui_set_accessibility_text_box(graphics::resource* control, const void* name, const void* help) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_accessibility(require_node(control), detail::text_value(name), detail::text_value(help));
    });
}

extern "C" int txrt_gui_set_label_text_box(graphics::resource* control, graphics::resource* label) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_label(require_node(control), require_node(label));
    });
}

extern "C" int txrt_gui_set_accessibility_check_box(graphics::resource* control, const void* name, const void* help) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_accessibility(require_node(control), detail::text_value(name), detail::text_value(help));
    });
}

extern "C" int txrt_gui_set_label_check_box(graphics::resource* control, graphics::resource* label) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_label(require_node(control), require_node(label));
    });
}

extern "C" int txrt_gui_set_accessibility_list_view(graphics::resource* control, const void* name, const void* help) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_accessibility(require_node(control), detail::text_value(name), detail::text_value(help));
    });
}

extern "C" int txrt_gui_set_label_list_view(graphics::resource* control, graphics::resource* label) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_label(require_node(control), require_node(label));
    });
}

extern "C" int txrt_gui_set_accessibility_table_view(graphics::resource* control, const void* name, const void* help) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_accessibility(require_node(control), detail::text_value(name), detail::text_value(help));
    });
}

extern "C" int txrt_gui_set_label_table_view(graphics::resource* control, graphics::resource* label) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_label(require_node(control), require_node(label));
    });
}

extern "C" int txrt_gui_set_accessibility_tree_view(graphics::resource* control, const void* name, const void* help) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_accessibility(require_node(control), detail::text_value(name), detail::text_value(help));
    });
}

extern "C" int txrt_gui_set_label_tree_view(graphics::resource* control, graphics::resource* label) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_label(require_node(control), require_node(label));
    });
}

extern "C" int txrt_gui_set_accessibility_progress_bar(graphics::resource* control, const void* name, const void* help) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_accessibility(require_node(control), detail::text_value(name), detail::text_value(help));
    });
}

extern "C" int txrt_gui_set_label_progress_bar(graphics::resource* control, graphics::resource* label) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_label(require_node(control), require_node(label));
    });
}

extern "C" int txrt_gui_set_accessibility_slider(graphics::resource* control, const void* name, const void* help) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_accessibility(require_node(control), detail::text_value(name), detail::text_value(help));
    });
}

extern "C" int txrt_gui_set_label_slider(graphics::resource* control, graphics::resource* label) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_label(require_node(control), require_node(label));
    });
}

extern "C" int txrt_gui_set_accessibility_tabs(graphics::resource* control, const void* name, const void* help) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_accessibility(require_node(control), detail::text_value(name), detail::text_value(help));
    });
}

extern "C" int txrt_gui_set_label_tabs(graphics::resource* control, graphics::resource* label) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_label(require_node(control), require_node(label));
    });
}

extern "C" int txrt_gui_set_accessibility_canvas(graphics::resource* control, const void* name, const void* help) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_accessibility(require_node(control), detail::text_value(name), detail::text_value(help));
    });
}

extern "C" int txrt_gui_set_label_canvas(graphics::resource* control, graphics::resource* label) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_label(require_node(control), require_node(label));
    });
}
