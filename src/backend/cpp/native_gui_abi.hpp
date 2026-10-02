#pragma once

#include "stdlib/graphics/resource.hpp"
#include <cstdint>

namespace tx_generated
{
struct record_type;
}

extern "C"
{
int txrt_native_gui_open_app(const char* type, void** result) noexcept;
int txrt_native_gui_open_app_with_font(const void* font, const char* type, void** result) noexcept;
int txrt_native_gui_create_window(tx_generated::graphics::resource* app, const void* title,
    double width, double height, const char* type, void** result) noexcept;
int txrt_native_gui_show(tx_generated::graphics::resource* window) noexcept;
int txrt_native_gui_set_title(tx_generated::graphics::resource* window, const void* title) noexcept;
int txrt_native_gui_is_open(tx_generated::graphics::resource* window, bool* result) noexcept;
int txrt_native_gui_close_window(tx_generated::graphics::resource* window) noexcept;
int txrt_native_gui_close_app(tx_generated::graphics::resource* app) noexcept;
int txrt_native_gui_next_event(tx_generated::graphics::resource* app, std::int64_t timeout,
    const char* type, const char* event_type, void** result) noexcept;
int txrt_native_gui_root(tx_generated::graphics::resource* window, const char* type, void** result) noexcept;
int txrt_native_gui_create_panel(tx_generated::graphics::resource* parent, const char* type, void** result) noexcept;
int txrt_native_gui_create_label(tx_generated::graphics::resource* parent, const void* text, const char* type, void** result) noexcept;
int txrt_native_gui_create_button(tx_generated::graphics::resource* parent, const void* text, const char* type, void** result) noexcept;
int txrt_native_gui_create_check_box(tx_generated::graphics::resource* parent, const void* text, const char* type, void** result) noexcept;
int txrt_native_gui_set_column(tx_generated::graphics::resource* panel, double padding, double gap) noexcept;
int txrt_native_gui_set_row(tx_generated::graphics::resource* panel, double padding, double gap) noexcept;
int txrt_native_gui_set_theme(tx_generated::graphics::resource* panel, const void* theme) noexcept;
int txrt_native_gui_paint(tx_generated::graphics::resource* panel, const char* type, void** result) noexcept;
int txrt_native_gui_save_bitmap(tx_generated::graphics::resource* panel, const void* path, const char* type, void** result) noexcept;
int txrt_native_gui_set_checked(tx_generated::graphics::resource* control, bool checked) noexcept;
int txrt_native_gui_checked(tx_generated::graphics::resource* control, bool* result) noexcept;
int txrt_native_gui_create_text_box(tx_generated::graphics::resource* parent, bool multiline, const char* type, void** result) noexcept;
int txrt_native_gui_text(tx_generated::graphics::resource* control, void** result) noexcept;
int txrt_native_gui_set_read_only(tx_generated::graphics::resource* control, bool read_only) noexcept;
int txrt_native_gui_set_password(tx_generated::graphics::resource* control, bool password) noexcept;
int txrt_native_gui_set_text_limit(tx_generated::graphics::resource* control, std::int64_t limit) noexcept;
int txrt_native_gui_set_selection(tx_generated::graphics::resource* control, std::int64_t start, std::int64_t end) noexcept;
int txrt_native_gui_selection_start(tx_generated::graphics::resource* control, std::int64_t* result) noexcept;
int txrt_native_gui_selection_end(tx_generated::graphics::resource* control, std::int64_t* result) noexcept;
int txrt_native_gui_create_progress_bar(tx_generated::graphics::resource* parent, const char* type, void** result) noexcept;
int txrt_native_gui_set_progress(tx_generated::graphics::resource* control, double minimum, double maximum, double value) noexcept;
int txrt_native_gui_progress_value(tx_generated::graphics::resource* control, double* result) noexcept;
int txrt_native_gui_set_indeterminate(tx_generated::graphics::resource* control, bool enabled) noexcept;
int txrt_native_gui_create_slider(tx_generated::graphics::resource* parent, double minimum, double maximum,
    double value, double step, const char* type, void** result) noexcept;
int txrt_native_gui_set_slider_value(tx_generated::graphics::resource* control, double value) noexcept;
int txrt_native_gui_slider_value(tx_generated::graphics::resource* control, double* result) noexcept;
int txrt_native_gui_create_radio_button(tx_generated::graphics::resource* parent, const void* text, std::int64_t group,
    const char* type, void** result) noexcept;
int txrt_native_gui_set_radio_selected(tx_generated::graphics::resource* control, bool selected) noexcept;
int txrt_native_gui_radio_selected(tx_generated::graphics::resource* control, bool* result) noexcept;
int txrt_native_gui_fixed(double value, const tx_generated::record_type* type, void** result) noexcept;
int txrt_native_gui_auto_length(const tx_generated::record_type* type, void** result) noexcept;
int txrt_native_gui_stretch(double weight, const tx_generated::record_type* type, void** result) noexcept;
int txrt_native_gui_set_grid(tx_generated::graphics::resource* panel, std::int64_t rows,
    std::int64_t columns, double padding, double gap) noexcept;
int txrt_native_gui_set_grid_row(tx_generated::graphics::resource* panel, std::int64_t index,
    const void* mode, double value) noexcept;
int txrt_native_gui_set_grid_column(tx_generated::graphics::resource* panel, std::int64_t index,
    const void* mode, double value) noexcept;
int txrt_native_gui_set_overlay(tx_generated::graphics::resource* panel, double padding) noexcept;

#define tx_native_gui_style_declare(kind) \
int txrt_native_gui_set_width_##kind(tx_generated::graphics::resource* control, const void* mode, double value) noexcept; \
int txrt_native_gui_set_height_##kind(tx_generated::graphics::resource* control, const void* mode, double value) noexcept; \
int txrt_native_gui_set_constraints_##kind(tx_generated::graphics::resource* control, double a, double b, double c, double d) noexcept; \
int txrt_native_gui_set_margin_##kind(tx_generated::graphics::resource* control, double margin) noexcept; \
int txrt_native_gui_set_alignment_##kind(tx_generated::graphics::resource* control, const void* x, const void* y) noexcept; \
int txrt_native_gui_set_cell_##kind(tx_generated::graphics::resource* control, std::int64_t row, std::int64_t col, std::int64_t rs, std::int64_t cs) noexcept;

tx_native_gui_style_declare(panel)
tx_native_gui_style_declare(label)
tx_native_gui_style_declare(button)
tx_native_gui_style_declare(check_box)
tx_native_gui_style_declare(text_box)
tx_native_gui_style_declare(progress_bar)
tx_native_gui_style_declare(slider)
tx_native_gui_style_declare(radio_button)
#undef tx_native_gui_style_declare

#define tx_native_gui_declare(kind) \
int txrt_native_gui_id_##kind(tx_generated::graphics::resource* control, std::int64_t* result) noexcept; \
int txrt_native_gui_close_##kind(tx_generated::graphics::resource* control) noexcept; \
int txrt_native_gui_set_size_##kind(tx_generated::graphics::resource* control, double width, double height) noexcept; \
int txrt_native_gui_set_visible_##kind(tx_generated::graphics::resource* control, bool visible) noexcept; \
int txrt_native_gui_set_enabled_##kind(tx_generated::graphics::resource* control, bool enabled) noexcept;

tx_native_gui_declare(panel)
tx_native_gui_declare(label)
tx_native_gui_declare(button)
tx_native_gui_declare(check_box)
tx_native_gui_declare(text_box)
tx_native_gui_declare(progress_bar)
tx_native_gui_declare(slider)
tx_native_gui_declare(radio_button)
#undef tx_native_gui_declare

int txrt_native_gui_set_text_label(tx_generated::graphics::resource* control, const void* text) noexcept;
int txrt_native_gui_set_text_button(tx_generated::graphics::resource* control, const void* text) noexcept;
int txrt_native_gui_set_text_check_box(tx_generated::graphics::resource* control, const void* text) noexcept;
int txrt_native_gui_set_text_text_box(tx_generated::graphics::resource* control, const void* text) noexcept;
int txrt_native_gui_set_text_radio_button(tx_generated::graphics::resource* control, const void* text) noexcept;
}
