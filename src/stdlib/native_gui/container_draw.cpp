#include "stdlib/native_gui/drawing.hpp"

namespace tx_generated::native_gui
{
void draw_scrollbars(node& state, tx::ui::rasterizer& painter, const palette& theme)
{
    if (!state.scroll)
    {
        return;
    }
    const auto& scroll = *state.scroll;
    for (const auto& bounds : {scroll.horizontal_bar, scroll.vertical_bar})
    {
        if (bounds.width > 0 && bounds.height > 0)
        {
            painter.fill_rect(bounds, theme.background);
        }
    }
    for (const auto& bounds : {scroll.horizontal_thumb, scroll.vertical_thumb})
    {
        if (bounds.width > 0 && bounds.height > 0)
        {
            painter.fill_rounded_rect(bounds, 4, scroll.drag_axis ? theme.accent : theme.muted);
        }
    }
}

void draw_container(node& state, tx::ui::rasterizer& painter, const palette& theme)
{
    if (state.tabs)
    {
        painter.push_clip({state.bounds.x, state.bounds.y, state.bounds.width, std::min(36.0, state.bounds.height)});
        for (const auto& [page, bounds] : state.tabs->headers)
        {
            const bool selected = page->id == state.tabs->selected;
            painter.fill_rect(bounds, selected ? theme.panel : theme.background);
            if (selected)
            {
                painter.fill_rect({bounds.x, bounds.y + bounds.height - 3, bounds.width, 3}, theme.accent);
            }
            painter.push_clip({bounds.x + 12, bounds.y, std::max(0.0, bounds.width - 24), bounds.height});
            if (page->text_layout)
            {
                page->text_layout->draw(painter, {bounds.x + 16,
                    bounds.y + std::max(0.0, (bounds.height - page->text_layout->height()) / 2)},
                    page->enabled ? theme.foreground : theme.muted);
            }
            painter.pop_clip();
        }
        painter.pop_clip();
    }
    if (state.split)
    {
        painter.fill_rect(state.split->handle, state.split->dragging ? theme.accent : theme.border);
    }
    if (state.canvas)
    {
        painter.fill_rect(state.bounds, state.canvas->background);
        for (const auto& item : state.canvas->items)
        {
            const tx::ui::rect bounds{state.bounds.x + item.bounds.x, state.bounds.y + item.bounds.y,
                item.bounds.width, item.bounds.height};
            if (item.operation == canvas_operation::rectangle)
            {
                painter.fill_rect(bounds, item.color);
            }
            else if (item.operation == canvas_operation::ellipse)
            {
                painter.fill_ellipse(bounds, item.color);
            }
            else if (item.operation == canvas_operation::line)
            {
                painter.line({bounds.x, bounds.y}, {state.bounds.x + item.bounds.width,
                    state.bounds.y + item.bounds.height}, item.width, item.color);
            }
            else if (item.text)
            {
                item.text->draw(painter, {bounds.x, bounds.y}, item.color);
            }
        }
    }
}
}
