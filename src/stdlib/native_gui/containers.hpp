#pragma once

#include "stdlib/native_gui/state.hpp"

namespace tx_generated::native_gui
{
std::shared_ptr<node> create_scroll(node& parent, bool horizontal, bool vertical);
std::shared_ptr<node> create_tabs(node& parent);
std::shared_ptr<node> add_tab(node& state, const std::string& title);
std::shared_ptr<node> create_split(node& parent, bool horizontal, double ratio);
std::shared_ptr<node> content_panel(node& state, std::size_t index);
void normalize_tabs(node& state);
void select_tab(node& state, std::int64_t id, bool notify);
void set_split_ratio(node& state, double ratio, bool notify);
bool scroll_to(node& state, double x, double y, bool notify);
void arrange_container(node& state);
void update_scroll_geometry(scroll_state& state, tx::ui::rect bounds, double width, double height);
void reveal_node(node& state);
bool container_hit(const node& state, double x, double y);
bool container_pointer(node& state, const tx::ui::window_event& event);
bool container_key(node& state, const tx::ui::window_event& event);
bool switch_tab_from_focus(node& root, const tx::ui::window_event& event);
bool wheel_scroll(node& state, const tx::ui::window_event& event);
void cancel_container_interaction(node& state) noexcept;
void canvas_event(node& state, const tx::ui::window_event& input);
}
