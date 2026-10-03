#include "backend/cpp/native_gui_system_abi.hpp"
#include "backend/cpp/native_gui_result.hpp"
#include "stdlib/native_gui/commands.hpp"
#include "stdlib/native_gui/dialogs.hpp"
#include "stdlib/native_gui/theme.hpp"
#include "stdlib/native_gui/accessibility.hpp"

using namespace tx_generated;
using graphics::resource;

extern "C" int txrt_native_gui_bind_button_command(resource* button, resource* command) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::bind_command(native_gui::require_node(button), native_gui::require_command(command));
    });
}

extern "C" int txrt_native_gui_create_command_button(resource* parent, resource* command, const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        auto& action = native_gui::require_command(command);
        auto& panel = native_gui::require_node(parent);
        if (panel.window.lock() != action.owner.lock())
        {
            native_gui::fail("wrong_owner", "命令与按钮必须属于同一窗口");
        }
        auto button = native_gui::create(panel, tx::graphics_kind::native_button, action.text);
        native_gui::bind_command(*button, action);
        return graphics::make_handle(button);
    });
}

extern "C" int txrt_native_gui_create_toolbar(resource* parent, const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        auto panel = native_gui::create(native_gui::require_node(parent), tx::graphics_kind::native_panel, "");
        panel->layout = native_gui::layout_mode::row;
        panel->semantic_role = "toolbar";
        panel->padding = 4;
        return graphics::make_handle(panel);
    });
}

extern "C" int txrt_native_gui_create_status_bar(resource* parent, const void* text, const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        auto label = native_gui::create(native_gui::require_node(parent), tx::graphics_kind::native_label, detail::text_value(text));
        label->semantic_role = "status";
        return graphics::make_handle(label);
    });
}

extern "C" int txrt_native_gui_show_modal(resource* dialog, resource* owner) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::show_modal(native_gui::require_window(dialog), native_gui::require_window(owner));
    });
}

extern "C" int txrt_native_gui_end_dialog(resource* dialog, std::int64_t value) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::end_dialog(native_gui::require_window(dialog), value);
    });
}

extern "C" int txrt_native_gui_dialog_result(resource* dialog, const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        const auto& state = native_gui::require_window(dialog, true);
        return graphics::option_value("option<int>", state.dialog_result ? std::any(*state.dialog_result) : std::any{});
    });
}

extern "C" int txrt_native_gui_message_box(resource* window, const void* title, const void* text, const void* buttons, const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        return native_gui::message_box(native_gui::require_window(window), detail::text_value(title), detail::text_value(text), detail::text_value(buttons));
    });
}

extern "C" int txrt_native_gui_file_dialog(resource* window, const void* kind, const void* title, const void* initial, const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        const auto value = native_gui::file_dialog(native_gui::require_window(window), detail::text_value(kind), detail::text_value(title), detail::text_value(initial));
        return graphics::option_value("option<str>", value ? std::any(*value) : std::any{});
    });
}

extern "C" int txrt_native_gui_set_font_size(resource* panel, double size) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::set_font_size(native_gui::require_node(panel), size);
    });
}

extern "C" int txrt_native_gui_set_ui_scale(resource* panel, double scale) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::set_ui_scale(native_gui::require_node(panel), scale);
    });
}

extern "C" int txrt_native_gui_set_accessibility_panel(resource* control, const void* name, const void* help) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::set_accessibility(native_gui::require_node(control), detail::text_value(name), detail::text_value(help));
    });
}

extern "C" int txrt_native_gui_set_accessibility_label(resource* control, const void* name, const void* help) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::set_accessibility(native_gui::require_node(control), detail::text_value(name), detail::text_value(help));
    });
}

extern "C" int txrt_native_gui_set_accessibility_button(resource* control, const void* name, const void* help) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::set_accessibility(native_gui::require_node(control), detail::text_value(name), detail::text_value(help));
    });
}

extern "C" int txrt_native_gui_set_accessibility_check_box(resource* control, const void* name, const void* help) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::set_accessibility(native_gui::require_node(control), detail::text_value(name), detail::text_value(help));
    });
}

extern "C" int txrt_native_gui_set_accessibility_text_box(resource* control, const void* name, const void* help) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::set_accessibility(native_gui::require_node(control), detail::text_value(name), detail::text_value(help));
    });
}

extern "C" int txrt_native_gui_set_accessibility_progress_bar(resource* control, const void* name, const void* help) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::set_accessibility(native_gui::require_node(control), detail::text_value(name), detail::text_value(help));
    });
}

extern "C" int txrt_native_gui_set_accessibility_slider(resource* control, const void* name, const void* help) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::set_accessibility(native_gui::require_node(control), detail::text_value(name), detail::text_value(help));
    });
}

extern "C" int txrt_native_gui_set_accessibility_radio_button(resource* control, const void* name, const void* help) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::set_accessibility(native_gui::require_node(control), detail::text_value(name), detail::text_value(help));
    });
}

extern "C" int txrt_native_gui_set_accessibility_combo_box(resource* control, const void* name, const void* help) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::set_accessibility(native_gui::require_node(control), detail::text_value(name), detail::text_value(help));
    });
}

extern "C" int txrt_native_gui_set_accessibility_scroll(resource* control, const void* name, const void* help) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::set_accessibility(native_gui::require_node(control), detail::text_value(name), detail::text_value(help));
    });
}

extern "C" int txrt_native_gui_set_accessibility_tabs(resource* control, const void* name, const void* help) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::set_accessibility(native_gui::require_node(control), detail::text_value(name), detail::text_value(help));
    });
}

extern "C" int txrt_native_gui_set_accessibility_split(resource* control, const void* name, const void* help) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::set_accessibility(native_gui::require_node(control), detail::text_value(name), detail::text_value(help));
    });
}

extern "C" int txrt_native_gui_set_accessibility_canvas(resource* control, const void* name, const void* help) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::set_accessibility(native_gui::require_node(control), detail::text_value(name), detail::text_value(help));
    });
}

extern "C" int txrt_native_gui_set_accessibility_list_view(resource* control, const void* name, const void* help) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::set_accessibility(native_gui::require_node(control), detail::text_value(name), detail::text_value(help));
    });
}

extern "C" int txrt_native_gui_set_accessibility_table_view(resource* control, const void* name, const void* help) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::set_accessibility(native_gui::require_node(control), detail::text_value(name), detail::text_value(help));
    });
}

extern "C" int txrt_native_gui_set_accessibility_tree_view(resource* control, const void* name, const void* help) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::set_accessibility(native_gui::require_node(control), detail::text_value(name), detail::text_value(help));
    });
}
