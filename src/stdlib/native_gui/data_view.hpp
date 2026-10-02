#pragma once

#include "stdlib/native_gui/data_model.hpp"
#include "stdlib/native_gui/core/text_layout.hpp"
#include "stdlib/native_gui/platform/window.hpp"

#include <map>
#include <set>
#include <memory>

namespace tx_generated::native_gui
{
struct node;
struct palette;

struct visible_row
{
    std::size_t index = 0;
    unsigned depth = 0;
};

struct data_view_state
{
    explicit data_view_state(tx::ui::data_kind kind, std::vector<tx::ui::data_column> columns)
        : model(kind, std::move(columns))
    {
    }
    tx::ui::data_model model;
    bool multiple = false, rebuild = true, sort_ascending = true;
    std::int64_t cursor = 0, anchor = 0, sort_column = 0, focus_column = 0;
    std::int64_t cached_revision = -1;
    double row_height = 32;
    std::set<std::int64_t> selected, expanded;
    std::vector<visible_row> visible;
    std::unordered_map<std::int64_t, std::size_t> positions;
    std::size_t first = 0, last = 0;
    std::map<std::pair<std::int64_t, std::int64_t>, std::unique_ptr<tx::ui::text_layout>> text_cache;
    std::int64_t resizing_column = 0, pressed_column = 0;
    double resize_x = 0, resize_width = 0;
};

std::shared_ptr<node> create_data_view(node& parent, tx::ui::data_kind kind, bool multiple,
    std::vector<tx::ui::data_column> columns = {});
void refresh_data(node& state);
void arrange_data(node& state);
void draw_data(node& state, tx::ui::rasterizer& painter, const palette& theme);
bool data_pointer(node& state, const tx::ui::window_event& input);
bool data_key(node& state, const tx::ui::window_event& input);
bool set_data_selection(node& state, const std::vector<std::int64_t>& ids, bool notify);
void select_data_row(node& state, std::int64_t id, bool extend, bool toggle, bool notify);
void set_expanded(node& state, std::int64_t id, bool expanded, bool notify);
void reveal_data_cursor(node& state);
void request_data_sort(node& state, std::int64_t column_id);
void data_notification(node& state, const char* kind, std::int64_t id = 0, bool value = false);
}
