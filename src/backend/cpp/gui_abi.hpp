#pragma once

#include "stdlib/graphics/resource.hpp"
#include <cstdint>

namespace tx_generated
{
struct record_type;
}

extern "C"
{
int txrt_gui_fixed(double value, const tx_generated::record_type* result_type, void** result) noexcept;
int txrt_gui_auto_length(const tx_generated::record_type* result_type, void** result) noexcept;
int txrt_gui_stretch(double weight, const tx_generated::record_type* result_type, void** result) noexcept;
int txrt_gui_root(tx_generated::graphics::resource* window, const char* result_type, void** result) noexcept;
int txrt_gui_create_container(tx_generated::graphics::resource* parent, const char* result_type, void** result) noexcept;
int txrt_gui_create_label(tx_generated::graphics::resource* parent, const void* text, const char* result_type, void** result) noexcept;
int txrt_gui_create_button(tx_generated::graphics::resource* parent, const void* text, const char* result_type, void** result) noexcept;
int txrt_gui_create_text_box(tx_generated::graphics::resource* parent, bool multiline, const char* result_type, void** result) noexcept;
int txrt_gui_create_check_box(tx_generated::graphics::resource* parent, const void* text, bool three_state, const char* result_type, void** result) noexcept;
int txrt_gui_set_column(tx_generated::graphics::resource* parent, double padding, double gap) noexcept;
int txrt_gui_set_theme(tx_generated::graphics::resource* root, const void* theme) noexcept;
int txrt_gui_set_button_appearance(tx_generated::graphics::resource* control, const void* appearance) noexcept;
int txrt_gui_set_row(tx_generated::graphics::resource* parent, double padding, double gap) noexcept;
int txrt_gui_set_grid(tx_generated::graphics::resource* parent, std::int64_t rows, std::int64_t columns, double padding, double gap) noexcept;
int txrt_gui_set_grid_row(tx_generated::graphics::resource* parent, std::int64_t index, const void* value_mode, double value_value) noexcept;
int txrt_gui_set_grid_column(tx_generated::graphics::resource* parent, std::int64_t index, const void* value_mode, double value_value) noexcept;
int txrt_gui_flush_layout(tx_generated::graphics::resource* window) noexcept;
int txrt_gui_set_default_button(tx_generated::graphics::resource* control) noexcept;
int txrt_gui_set_cancel_button(tx_generated::graphics::resource* control) noexcept;
int txrt_gui_set_checked(tx_generated::graphics::resource* control, bool checked) noexcept;
int txrt_gui_checked(tx_generated::graphics::resource* control, bool* result) noexcept;
int txrt_gui_set_check_state(tx_generated::graphics::resource* control, const void* state) noexcept;
int txrt_gui_check_state(tx_generated::graphics::resource* control, void** result) noexcept;
int txrt_gui_set_read_only(tx_generated::graphics::resource* control, bool read_only) noexcept;
int txrt_gui_set_password(tx_generated::graphics::resource* control, bool password) noexcept;
int txrt_gui_set_text_limit(tx_generated::graphics::resource* control, std::int64_t limit) noexcept;
int txrt_gui_set_selection(tx_generated::graphics::resource* control, std::int64_t start, std::int64_t end) noexcept;
int txrt_gui_set_text_follow_end(tx_generated::graphics::resource* control, const void* text) noexcept;
int txrt_gui_selection_start(tx_generated::graphics::resource* control, std::int64_t* result) noexcept;
int txrt_gui_selection_end(tx_generated::graphics::resource* control, std::int64_t* result) noexcept;
int txrt_gui_id_container(tx_generated::graphics::resource* control, std::int64_t* result) noexcept;
int txrt_gui_is_open_container(tx_generated::graphics::resource* control, bool* result) noexcept;
int txrt_gui_close_container(tx_generated::graphics::resource* control) noexcept;
int txrt_gui_set_enabled_container(tx_generated::graphics::resource* control, bool enabled) noexcept;
int txrt_gui_set_visible_container(tx_generated::graphics::resource* control, bool visible) noexcept;
int txrt_gui_set_reserved_space_container(tx_generated::graphics::resource* control, bool reserved) noexcept;
int txrt_gui_set_width_container(tx_generated::graphics::resource* control, const void* value_mode, double value_value) noexcept;
int txrt_gui_set_height_container(tx_generated::graphics::resource* control, const void* value_mode, double value_value) noexcept;
int txrt_gui_set_constraints_container(tx_generated::graphics::resource* control, double min_width, double min_height, double max_width, double max_height) noexcept;
int txrt_gui_set_margin_container(tx_generated::graphics::resource* control, double margin) noexcept;
int txrt_gui_set_alignment_container(tx_generated::graphics::resource* control, const void* horizontal, const void* vertical) noexcept;
int txrt_gui_set_cell_container(tx_generated::graphics::resource* control, std::int64_t row, std::int64_t column, std::int64_t row_span, std::int64_t column_span) noexcept;
int txrt_gui_id_label(tx_generated::graphics::resource* control, std::int64_t* result) noexcept;
int txrt_gui_is_open_label(tx_generated::graphics::resource* control, bool* result) noexcept;
int txrt_gui_close_label(tx_generated::graphics::resource* control) noexcept;
int txrt_gui_set_enabled_label(tx_generated::graphics::resource* control, bool enabled) noexcept;
int txrt_gui_set_visible_label(tx_generated::graphics::resource* control, bool visible) noexcept;
int txrt_gui_set_reserved_space_label(tx_generated::graphics::resource* control, bool reserved) noexcept;
int txrt_gui_set_width_label(tx_generated::graphics::resource* control, const void* value_mode, double value_value) noexcept;
int txrt_gui_set_height_label(tx_generated::graphics::resource* control, const void* value_mode, double value_value) noexcept;
int txrt_gui_set_constraints_label(tx_generated::graphics::resource* control, double min_width, double min_height, double max_width, double max_height) noexcept;
int txrt_gui_set_margin_label(tx_generated::graphics::resource* control, double margin) noexcept;
int txrt_gui_set_alignment_label(tx_generated::graphics::resource* control, const void* horizontal, const void* vertical) noexcept;
int txrt_gui_set_cell_label(tx_generated::graphics::resource* control, std::int64_t row, std::int64_t column, std::int64_t row_span, std::int64_t column_span) noexcept;
int txrt_gui_set_text_label(tx_generated::graphics::resource* control, const void* text) noexcept;
int txrt_gui_text_label(tx_generated::graphics::resource* control, void** result) noexcept;
int txrt_gui_id_button(tx_generated::graphics::resource* control, std::int64_t* result) noexcept;
int txrt_gui_is_open_button(tx_generated::graphics::resource* control, bool* result) noexcept;
int txrt_gui_close_button(tx_generated::graphics::resource* control) noexcept;
int txrt_gui_set_enabled_button(tx_generated::graphics::resource* control, bool enabled) noexcept;
int txrt_gui_set_visible_button(tx_generated::graphics::resource* control, bool visible) noexcept;
int txrt_gui_set_reserved_space_button(tx_generated::graphics::resource* control, bool reserved) noexcept;
int txrt_gui_set_width_button(tx_generated::graphics::resource* control, const void* value_mode, double value_value) noexcept;
int txrt_gui_set_height_button(tx_generated::graphics::resource* control, const void* value_mode, double value_value) noexcept;
int txrt_gui_set_constraints_button(tx_generated::graphics::resource* control, double min_width, double min_height, double max_width, double max_height) noexcept;
int txrt_gui_set_margin_button(tx_generated::graphics::resource* control, double margin) noexcept;
int txrt_gui_set_alignment_button(tx_generated::graphics::resource* control, const void* horizontal, const void* vertical) noexcept;
int txrt_gui_set_cell_button(tx_generated::graphics::resource* control, std::int64_t row, std::int64_t column, std::int64_t row_span, std::int64_t column_span) noexcept;
int txrt_gui_set_text_button(tx_generated::graphics::resource* control, const void* text) noexcept;
int txrt_gui_text_button(tx_generated::graphics::resource* control, void** result) noexcept;
int txrt_gui_focus_button(tx_generated::graphics::resource* control, const char* result_type, void** result) noexcept;
int txrt_gui_id_text_box(tx_generated::graphics::resource* control, std::int64_t* result) noexcept;
int txrt_gui_is_open_text_box(tx_generated::graphics::resource* control, bool* result) noexcept;
int txrt_gui_close_text_box(tx_generated::graphics::resource* control) noexcept;
int txrt_gui_set_enabled_text_box(tx_generated::graphics::resource* control, bool enabled) noexcept;
int txrt_gui_set_visible_text_box(tx_generated::graphics::resource* control, bool visible) noexcept;
int txrt_gui_set_reserved_space_text_box(tx_generated::graphics::resource* control, bool reserved) noexcept;
int txrt_gui_set_width_text_box(tx_generated::graphics::resource* control, const void* value_mode, double value_value) noexcept;
int txrt_gui_set_height_text_box(tx_generated::graphics::resource* control, const void* value_mode, double value_value) noexcept;
int txrt_gui_set_constraints_text_box(tx_generated::graphics::resource* control, double min_width, double min_height, double max_width, double max_height) noexcept;
int txrt_gui_set_margin_text_box(tx_generated::graphics::resource* control, double margin) noexcept;
int txrt_gui_set_alignment_text_box(tx_generated::graphics::resource* control, const void* horizontal, const void* vertical) noexcept;
int txrt_gui_set_cell_text_box(tx_generated::graphics::resource* control, std::int64_t row, std::int64_t column, std::int64_t row_span, std::int64_t column_span) noexcept;
int txrt_gui_set_text_text_box(tx_generated::graphics::resource* control, const void* text) noexcept;
int txrt_gui_text_text_box(tx_generated::graphics::resource* control, void** result) noexcept;
int txrt_gui_focus_text_box(tx_generated::graphics::resource* control, const char* result_type, void** result) noexcept;
int txrt_gui_id_check_box(tx_generated::graphics::resource* control, std::int64_t* result) noexcept;
int txrt_gui_is_open_check_box(tx_generated::graphics::resource* control, bool* result) noexcept;
int txrt_gui_close_check_box(tx_generated::graphics::resource* control) noexcept;
int txrt_gui_set_enabled_check_box(tx_generated::graphics::resource* control, bool enabled) noexcept;
int txrt_gui_set_visible_check_box(tx_generated::graphics::resource* control, bool visible) noexcept;
int txrt_gui_set_reserved_space_check_box(tx_generated::graphics::resource* control, bool reserved) noexcept;
int txrt_gui_set_width_check_box(tx_generated::graphics::resource* control, const void* value_mode, double value_value) noexcept;
int txrt_gui_set_height_check_box(tx_generated::graphics::resource* control, const void* value_mode, double value_value) noexcept;
int txrt_gui_set_constraints_check_box(tx_generated::graphics::resource* control, double min_width, double min_height, double max_width, double max_height) noexcept;
int txrt_gui_set_margin_check_box(tx_generated::graphics::resource* control, double margin) noexcept;
int txrt_gui_set_alignment_check_box(tx_generated::graphics::resource* control, const void* horizontal, const void* vertical) noexcept;
int txrt_gui_set_cell_check_box(tx_generated::graphics::resource* control, std::int64_t row, std::int64_t column, std::int64_t row_span, std::int64_t column_span) noexcept;
int txrt_gui_set_text_check_box(tx_generated::graphics::resource* control, const void* text) noexcept;
int txrt_gui_text_check_box(tx_generated::graphics::resource* control, void** result) noexcept;
int txrt_gui_focus_check_box(tx_generated::graphics::resource* control, const char* result_type, void** result) noexcept;
}

