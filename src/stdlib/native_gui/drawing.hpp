#pragma once

#include "stdlib/native_gui/state.hpp"

namespace tx_generated::native_gui
{
struct palette
{
    tx::ui::color background, panel, foreground, muted, accent, hover, pressed, border;
};

void draw_container(node& state, tx::ui::rasterizer& painter, const palette& theme);
void draw_scrollbars(node& state, tx::ui::rasterizer& painter, const palette& theme);
}
