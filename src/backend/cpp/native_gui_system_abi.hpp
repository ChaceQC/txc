#pragma once

#include "stdlib/graphics/resource.hpp"
#include <cstdint>

extern "C"
{
int txrt_native_gui_create_command(tx_generated::graphics::resource* window, const void* text, const void* accelerator, bool checkable, const char* type, void** result) noexcept;
int txrt_native_gui_id_command(tx_generated::graphics::resource* command, std::int64_t* result) noexcept;
int txrt_native_gui_close_command(tx_generated::graphics::resource* command) noexcept;
int txrt_native_gui_set_command_state(tx_generated::graphics::resource* command, bool enabled, bool checked) noexcept;
int txrt_native_gui_command_enabled(tx_generated::graphics::resource* command, bool* result) noexcept;
int txrt_native_gui_command_checked(tx_generated::graphics::resource* command, bool* result) noexcept;
int txrt_native_gui_set_text_command(tx_generated::graphics::resource* command, const void* text) noexcept;
int txrt_native_gui_set_shortcut(tx_generated::graphics::resource* command, const void* accelerator) noexcept;
int txrt_native_gui_invoke_command(tx_generated::graphics::resource* command) noexcept;
int txrt_native_gui_create_menu(tx_generated::graphics::resource* window, const char* type, void** result) noexcept;
int txrt_native_gui_id_menu(tx_generated::graphics::resource* menu, std::int64_t* result) noexcept;
int txrt_native_gui_close_menu(tx_generated::graphics::resource* menu) noexcept;
int txrt_native_gui_append_menu_command(tx_generated::graphics::resource* menu, tx_generated::graphics::resource* command) noexcept;
int txrt_native_gui_append_submenu(tx_generated::graphics::resource* menu, const void* title, tx_generated::graphics::resource* submenu) noexcept;
int txrt_native_gui_append_separator(tx_generated::graphics::resource* menu) noexcept;
int txrt_native_gui_set_menu_bar(tx_generated::graphics::resource* window, tx_generated::graphics::resource* menu) noexcept;
int txrt_native_gui_popup_menu(tx_generated::graphics::resource* menu, double x, double y) noexcept;
int txrt_native_gui_dismiss_menu(tx_generated::graphics::resource* window) noexcept;
int txrt_native_gui_bind_button_command(tx_generated::graphics::resource* button, tx_generated::graphics::resource* command) noexcept;
int txrt_native_gui_create_command_button(tx_generated::graphics::resource* parent, tx_generated::graphics::resource* command, const char* type, void** result) noexcept;
int txrt_native_gui_create_toolbar(tx_generated::graphics::resource* parent, const char* type, void** result) noexcept;
int txrt_native_gui_create_status_bar(tx_generated::graphics::resource* parent, const void* text, const char* type, void** result) noexcept;
int txrt_native_gui_show_modal(tx_generated::graphics::resource* dialog, tx_generated::graphics::resource* owner) noexcept;
int txrt_native_gui_end_dialog(tx_generated::graphics::resource* dialog, std::int64_t value) noexcept;
int txrt_native_gui_dialog_result(tx_generated::graphics::resource* dialog, const char* type, void** result) noexcept;
int txrt_native_gui_message_box(tx_generated::graphics::resource* window, const void* title, const void* text, const void* buttons, const char* type, void** result) noexcept;
int txrt_native_gui_file_dialog(tx_generated::graphics::resource* window, const void* kind, const void* title, const void* initial, const char* type, void** result) noexcept;
int txrt_native_gui_set_font_size(tx_generated::graphics::resource* panel, double size) noexcept;
int txrt_native_gui_set_ui_scale(tx_generated::graphics::resource* panel, double scale) noexcept;
int txrt_native_gui_set_accessibility_panel(tx_generated::graphics::resource* control, const void* name, const void* help) noexcept;
int txrt_native_gui_set_accessibility_label(tx_generated::graphics::resource* control, const void* name, const void* help) noexcept;
int txrt_native_gui_set_accessibility_button(tx_generated::graphics::resource* control, const void* name, const void* help) noexcept;
int txrt_native_gui_set_accessibility_check_box(tx_generated::graphics::resource* control, const void* name, const void* help) noexcept;
int txrt_native_gui_set_accessibility_text_box(tx_generated::graphics::resource* control, const void* name, const void* help) noexcept;
int txrt_native_gui_set_accessibility_progress_bar(tx_generated::graphics::resource* control, const void* name, const void* help) noexcept;
int txrt_native_gui_set_accessibility_slider(tx_generated::graphics::resource* control, const void* name, const void* help) noexcept;
int txrt_native_gui_set_accessibility_radio_button(tx_generated::graphics::resource* control, const void* name, const void* help) noexcept;
int txrt_native_gui_set_accessibility_combo_box(tx_generated::graphics::resource* control, const void* name, const void* help) noexcept;
int txrt_native_gui_set_accessibility_scroll(tx_generated::graphics::resource* control, const void* name, const void* help) noexcept;
int txrt_native_gui_set_accessibility_tabs(tx_generated::graphics::resource* control, const void* name, const void* help) noexcept;
int txrt_native_gui_set_accessibility_split(tx_generated::graphics::resource* control, const void* name, const void* help) noexcept;
int txrt_native_gui_set_accessibility_canvas(tx_generated::graphics::resource* control, const void* name, const void* help) noexcept;
int txrt_native_gui_set_accessibility_list_view(tx_generated::graphics::resource* control, const void* name, const void* help) noexcept;
int txrt_native_gui_set_accessibility_table_view(tx_generated::graphics::resource* control, const void* name, const void* help) noexcept;
int txrt_native_gui_set_accessibility_tree_view(tx_generated::graphics::resource* control, const void* name, const void* help) noexcept;
}
