#pragma once

#include "stdlib/native_gui/state.hpp"
#include "stdlib/native_gui/drawing.hpp"

namespace tx_generated::native_gui
{
struct combo_state
{
    std::vector<std::u32string> items;
    std::int64_t selected = -1, candidate = -1;
    std::size_t first = 0;
    bool open = false, pointer_pressed = false;
};

void set_combo_items(node& state, std::vector<std::u32string> items);
void select_combo(node& state, std::int64_t index, bool notify);
void open_combo(node& state);
void close_combo(node& root) noexcept;
rectangle combo_popup(node& state);
bool combo_key(node& state, const tx::ui::window_event& input);
bool combo_pointer(node& root, const tx::ui::window_event& input);
void draw_combo(node& state, tx::ui::rasterizer& painter, const palette& theme);
void draw_combo_popup(node& root, tx::ui::rasterizer& painter, const palette& theme);
}
