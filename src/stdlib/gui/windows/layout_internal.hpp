#pragma once

#include "stdlib/gui/windows/state.hpp"

namespace tx_generated::gui
{

axis_item child_axis(const node& child, bool horizontal);
std::vector<axis_item> grid_axis(node& state, bool horizontal);
extent measure(node& state, double available_width);
void arrange(node& state, bounds rectangle);
bool participates(const node& state);

} // namespace tx_generated::gui
