#pragma once

#include "stdlib/native_gui/state.hpp"

namespace tx_generated::native_gui
{
struct grid_geometry
{
    std::vector<double> columns, rows;
};

gui::extent measure(node& state, double available_width);
grid_geometry grid_tracks(node& state, double width, double height = -1);
void arrange(node& state, rectangle bounds, rectangle clip);
void arrange_grid(node& state);
double text_inset(const node& state);
}
