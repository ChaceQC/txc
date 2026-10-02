#pragma once

#include "stdlib/native_gui/core/text_layout.hpp"

#include <memory>
#include <array>

namespace tx_generated::native_gui
{
struct node;

struct scroll_state
{
    bool horizontal = false, vertical = true;
    double x = 0, y = 0, width = 0, height = 0;
    tx::ui::rect viewport, horizontal_bar, vertical_bar, horizontal_thumb, vertical_thumb;
    int drag_axis = 0;
    double drag_origin = 0, drag_offset = 0;
};

struct tabs_state
{
    std::int64_t selected = 0;
    double offset = 0, width = 0;
    double viewport_width = -1;
    bool reveal_selected = true;
    std::vector<std::pair<std::shared_ptr<node>, tx::ui::rect>> headers;
};

struct split_state
{
    bool horizontal = true, dragging = false;
    double ratio = 0.5;
    tx::ui::rect handle;
    std::array<std::weak_ptr<node>, 2> panels;
};

enum class canvas_operation
{
    rectangle, ellipse, line, text
};

struct canvas_item
{
    canvas_operation operation;
    tx::ui::rect bounds;
    tx::ui::color color;
    double width = 1;
    std::unique_ptr<tx::ui::text_layout> text;
};

struct canvas_state
{
    tx::ui::color background{255, 255, 255, 255};
    std::vector<canvas_item> items;
};
}
