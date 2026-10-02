#pragma once

#include <string_view>

namespace tx
{

// 与不透明运行时资源的类型标签共用；不能由导入别名推断资源身份。
enum class graphics_kind
{
    app, window, canvas, image, surface, path, brush, font, text_layout,
    control, menu, container, label, button, text_box, check_box, combo_box,
    list_view, table_view, tree_view, progress_bar, slider, tabs, gui_canvas,
    command, list_model, table_model, tree_model,
    native_panel, native_label, native_button, native_check_box, native_app, native_window,
    native_text_box, native_progress_bar, native_slider, native_radio_button, native_combo_box,
    native_list_view, native_table_view, native_tree_view, native_tabs, native_canvas,
    native_command, native_menu, unknown
};

inline graphics_kind graphics_type_kind(std::string_view name) noexcept
{
    constexpr std::string_view names[] = {"graphics_app", "graphics_window",
        "graphics_canvas", "graphics_image", "graphics_surface", "graphics_path",
        "graphics_brush", "graphics_font", "graphics_text_layout", "gui_control", "gui_menu",
        "gui_container", "gui_label", "gui_button", "gui_text_box", "gui_check_box",
        "gui_combo_box", "gui_list_view", "gui_table_view", "gui_tree_view",
        "gui_progress_bar", "gui_slider", "gui_tabs", "gui_canvas", "gui_command",
        "gui_list_model", "gui_table_model", "gui_tree_model",
        "native_gui_panel", "native_gui_label", "native_gui_button", "native_gui_check_box",
        "native_gui_app", "native_gui_window", "native_gui_text_box", "native_gui_progress_bar",
        "native_gui_slider", "native_gui_radio_button", "native_gui_combo_box", "native_gui_list_view",
        "native_gui_table_view", "native_gui_tree_view", "native_gui_tabs", "native_gui_canvas",
        "native_gui_command", "native_gui_menu"};
    for (int index = 0; index < static_cast<int>(graphics_kind::unknown); ++index)
    {
        if (name == names[index])
        {
            return static_cast<graphics_kind>(index);
        }
    }
    return graphics_kind::unknown;
}

} // namespace tx
