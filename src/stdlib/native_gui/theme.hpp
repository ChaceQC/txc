#pragma once

#include "stdlib/native_gui/drawing.hpp"

namespace tx_generated::native_gui
{
struct system_theme
{
    bool dark = false, high_contrast = false;
    tx::ui::color background{0, 0, 0, 255}, foreground{255, 255, 255, 255},
        accent{255, 255, 0, 255}, muted{170, 170, 170, 255};
};

system_theme read_system_theme();
palette theme_palette(node& root);
void set_theme(node& state, const std::string& name);
void set_font_size(node& state, double size);
void set_ui_scale(node& state, double scale);
void poll_theme(app& state);
}
