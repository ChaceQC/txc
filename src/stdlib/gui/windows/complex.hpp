#pragma once

#include "stdlib/gui/windows/state.hpp"

namespace tx_generated::gui
{
std::shared_ptr<node> add_tab(node& state, const std::string& title);
void select_tab(node& state, node& page, bool user = false);
void remove_page(node& state, node& page) noexcept;
std::shared_ptr<node> create_scroll(node& parent, bool horizontal, bool vertical);
std::shared_ptr<node> create_split(node& parent, bool vertical, double position,
    double minimum_first, double minimum_second);
void set_split_position(node& state, double value, bool user = false);
void set_scroll_position(node& state, double x, double y);
void set_progress(node& state, std::int64_t minimum, std::int64_t maximum, std::int64_t value);
void set_slider_value(node& state, std::int64_t value, bool user = false);
void value_event(node& state, const char* action, double value);
bool complex_layout(node& state);
bool complex_message(node& state, UINT message, WPARAM wparam, LPARAM lparam, LRESULT& result);
bool complex_key(node& state, WPARAM key);
void sync_canvas(node& state);
std::shared_ptr<graphics::canvas> begin_canvas(node& state);
void canvas_paint(node& state);
void set_accessibility(node& state, const std::string& name, const std::string& help);
void set_label(node& state, node& label);
void update_accessibility(node& state);
void close_accessibility(node& state) noexcept;
LRESULT accessibility_object(node& state, WPARAM wparam, LPARAM lparam);
constexpr UINT accessibility_action = WM_APP + 41;
constexpr UINT accessibility_focus = WM_APP + 42;
} // namespace tx_generated::gui
