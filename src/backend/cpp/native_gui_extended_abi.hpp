#pragma once

#include "stdlib/graphics/resource.hpp"
#include <cstdint>

extern "C"
{
int txrt_native_gui_create_scroll(tx_generated::graphics::resource* parent, bool horizontal, bool vertical,
    const char* type, void** result) noexcept;

int txrt_native_gui_scroll_content(tx_generated::graphics::resource* control, const char* type, void** result) noexcept;

int txrt_native_gui_scroll_to(tx_generated::graphics::resource* control, double x, double y) noexcept;

int txrt_native_gui_scroll_x(tx_generated::graphics::resource* control, double* result) noexcept;

int txrt_native_gui_scroll_y(tx_generated::graphics::resource* control, double* result) noexcept;

int txrt_native_gui_create_tabs(tx_generated::graphics::resource* parent, const char* type, void** result) noexcept;

int txrt_native_gui_add_tab(tx_generated::graphics::resource* control, const void* title, const char* type, void** result) noexcept;

int txrt_native_gui_select_tab(tx_generated::graphics::resource* control, std::int64_t id) noexcept;

int txrt_native_gui_selected_tab(tx_generated::graphics::resource* control, std::int64_t* result) noexcept;

int txrt_native_gui_tab_count(tx_generated::graphics::resource* control, std::int64_t* result) noexcept;

int txrt_native_gui_set_tab_title(tx_generated::graphics::resource* control, std::int64_t id, const void* title) noexcept;

int txrt_native_gui_create_split(tx_generated::graphics::resource* parent, bool horizontal, double ratio,
    const char* type, void** result) noexcept;

int txrt_native_gui_split_first(tx_generated::graphics::resource* control, const char* type, void** result) noexcept;

int txrt_native_gui_split_second(tx_generated::graphics::resource* control, const char* type, void** result) noexcept;

int txrt_native_gui_set_split_ratio(tx_generated::graphics::resource* control, double ratio) noexcept;

int txrt_native_gui_split_ratio(tx_generated::graphics::resource* control, double* result) noexcept;

int txrt_native_gui_create_canvas(tx_generated::graphics::resource* parent, const char* type, void** result) noexcept;

int txrt_native_gui_canvas_clear(tx_generated::graphics::resource* control, std::int64_t red, std::int64_t green,
    std::int64_t blue, std::int64_t alpha) noexcept;

int txrt_native_gui_canvas_fill_rect(tx_generated::graphics::resource* control, double x, double y, double width, double height,
    std::int64_t red, std::int64_t green, std::int64_t blue, std::int64_t alpha) noexcept;

int txrt_native_gui_canvas_fill_ellipse(tx_generated::graphics::resource* control, double x, double y, double width, double height,
    std::int64_t red, std::int64_t green, std::int64_t blue, std::int64_t alpha) noexcept;

int txrt_native_gui_canvas_line(tx_generated::graphics::resource* control, double x1, double y1, double x2, double y2, double width,
    std::int64_t red, std::int64_t green, std::int64_t blue, std::int64_t alpha) noexcept;

int txrt_native_gui_canvas_text(tx_generated::graphics::resource* control, const void* text, double x, double y, double size,
    double width, std::int64_t red, std::int64_t green, std::int64_t blue, std::int64_t alpha) noexcept;

int txrt_native_gui_create_list_view(tx_generated::graphics::resource* parent, bool multiple, const char* type, void** result) noexcept;

int txrt_native_gui_create_tree_view(tx_generated::graphics::resource* parent, bool multiple, const char* type, void** result) noexcept;

int txrt_native_gui_create_table_view(tx_generated::graphics::resource* parent, const void* value, bool multiple,
    const char* type, void** result) noexcept;

int txrt_native_gui_set_column_width(tx_generated::graphics::resource* control, std::int64_t id, double width) noexcept;

int txrt_native_gui_set_column_visible(tx_generated::graphics::resource* control, std::int64_t id, bool visible) noexcept;

int txrt_native_gui_set_column_order(tx_generated::graphics::resource* control, const void* ids) noexcept;

int txrt_native_gui_set_expanded(tx_generated::graphics::resource* control, std::int64_t id, bool expanded) noexcept;

int txrt_native_gui_expanded(tx_generated::graphics::resource* control, std::int64_t id, bool* result) noexcept;

int txrt_native_gui_item_text_list_view(tx_generated::graphics::resource* control, std::int64_t id, void** result) noexcept;

int txrt_native_gui_item_text_tree_view(tx_generated::graphics::resource* control, std::int64_t id, void** result) noexcept;

int txrt_native_gui_cell_text(tx_generated::graphics::resource* control, std::int64_t id, std::int64_t column_id, void** result) noexcept;

#define tx_native_data_declare(kind) \
int txrt_native_gui_replace_items_##kind##_view(tx_generated::graphics::resource*, const void*) noexcept; \
int txrt_native_gui_append_items_##kind##_view(tx_generated::graphics::resource*, const void*) noexcept; \
int txrt_native_gui_update_items_##kind##_view(tx_generated::graphics::resource*, const void*) noexcept; \
int txrt_native_gui_remove_items_##kind##_view(tx_generated::graphics::resource*, const void*) noexcept; \
int txrt_native_gui_set_order_##kind##_view(tx_generated::graphics::resource*, const void*) noexcept; \
int txrt_native_gui_apply_page_##kind##_view(tx_generated::graphics::resource*, std::int64_t, std::int64_t, const void*) noexcept; \
int txrt_native_gui_revision_##kind##_view(tx_generated::graphics::resource*, std::int64_t*) noexcept; \
int txrt_native_gui_count_##kind##_view(tx_generated::graphics::resource*, std::int64_t*) noexcept; \
int txrt_native_gui_begin_page_##kind##_view(tx_generated::graphics::resource*, std::int64_t*) noexcept; \
int txrt_native_gui_selected_ids_##kind##_view(tx_generated::graphics::resource*, void**) noexcept; \
int txrt_native_gui_set_selected_ids_##kind##_view(tx_generated::graphics::resource*, const void*) noexcept; \
int txrt_native_gui_visible_ids_##kind##_view(tx_generated::graphics::resource*, void**) noexcept; \
int txrt_native_gui_set_row_height_##kind##_view(tx_generated::graphics::resource*, double) noexcept;

tx_native_data_declare(list)
tx_native_data_declare(table)
tx_native_data_declare(tree)
#undef tx_native_data_declare
}
