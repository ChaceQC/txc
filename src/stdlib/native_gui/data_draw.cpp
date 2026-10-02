#include "stdlib/native_gui/drawing.hpp"

namespace tx_generated::native_gui
{
namespace
{
void cell_text(node& state, tx::ui::rasterizer& painter, tx::ui::rect bounds, std::int64_t row, std::int64_t column,
    std::u32string_view text, int alignment, tx::ui::color color)
{
    if (tx::ui::intersect(bounds, state.clip).width <= 0)
    {
        return;
    }
    auto& cache = state.data->text_cache[{row, column}];
    if (!cache)
    {
        cache = std::make_unique<tx::ui::text_layout>(*root_node(state).font, text, 16, std::max(1.0, bounds.width - 16), false);
    }
    const double extra = std::max(0.0, bounds.width - 16 - cache->width());
    const double x = bounds.x + 8 + (alignment == 1 ? extra / 2 : alignment == 2 ? extra : 0);
    painter.push_clip(bounds);
    cache->draw(painter, {x, bounds.y + std::max(0.0, (bounds.height - cache->height()) / 2)}, color);
    painter.pop_clip();
}

void draw_headers(node& state, tx::ui::rasterizer& painter, const palette& theme)
{
    const auto& data = *state.data;
    if (data.model.kind() != tx::ui::data_kind::table)
    {
        return;
    }
    const tx::ui::rect header{state.bounds.x, state.bounds.y, state.scroll->viewport.width,
        state.scroll->viewport.y - state.bounds.y};
    painter.push_clip(header);
    painter.fill_rect(header, theme.background);
    double x = state.bounds.x - state.scroll->x;
    for (const auto index : data.model.column_order())
    {
        const auto& column = data.model.columns()[index];
        if (!column.visible)
        {
            continue;
        }
        const tx::ui::rect bounds{x, header.y, column.width, header.height};
        cell_text(state, painter, {x, header.y, std::max(0.0, column.width - 14), header.height}, 0,
            column.id, column.title, column.alignment, available(state) ? theme.foreground : theme.muted);
        painter.line({x + column.width - 1, header.y}, {x + column.width - 1, header.y + header.height}, 1, theme.border);
        if (data.sort_column == column.id)
        {
            const double center = x + column.width - 9, y = header.y + header.height / 2;
            const double direction = data.sort_ascending ? -1 : 1;
            painter.line({center - 3, y - direction * 2}, {center, y + direction * 2}, 1, theme.accent);
            painter.line({center, y + direction * 2}, {center + 3, y - direction * 2}, 1, theme.accent);
        }
        if (root_node(state).focused.lock().get() == &state && data.focus_column == column.id)
        {
            painter.fill_rect({bounds.x, bounds.y + bounds.height - 2, bounds.width, 2}, theme.accent);
        }
        x += column.width;
    }
    painter.pop_clip();
}

void tree_marker(node& state, const visible_row& visible, const tx::ui::data_row& row,
    tx::ui::rect bounds, tx::ui::rasterizer& painter, const palette& theme)
{
    if (!row.has_children && !state.data->model.snapshot().children.contains(row.id))
    {
        return;
    }
    const double x = bounds.x + visible.depth * 18 + 12, y = bounds.y + bounds.height / 2;
    if (state.data->expanded.contains(row.id))
    {
        painter.line({x - 4, y - 2}, {x, y + 2}, 1.5, theme.foreground);
        painter.line({x, y + 2}, {x + 4, y - 2}, 1.5, theme.foreground);
    }
    else
    {
        painter.line({x - 2, y - 4}, {x + 2, y}, 1.5, theme.foreground);
        painter.line({x + 2, y}, {x - 2, y + 4}, 1.5, theme.foreground);
    }
}
}

void draw_data(node& state, tx::ui::rasterizer& painter, const palette& theme)
{
    if (!state.data)
    {
        return;
    }
    const auto& data = *state.data;
    const auto& scroll = *state.scroll;
    painter.fill_rect(state.bounds, theme.panel);
    draw_headers(state, painter, theme);
    painter.push_clip(scroll.viewport);
    for (auto index = data.first; index < data.last; ++index)
    {
        const auto& visible = data.visible[index];
        const auto& row = data.model.snapshot().rows[visible.index];
        const double y = scroll.viewport.y + index * data.row_height - scroll.y;
        const tx::ui::rect bounds{scroll.viewport.x, y, scroll.viewport.width, data.row_height};
        if (data.selected.contains(row.id))
        {
            painter.fill_rect(bounds, theme.hover);
        }
        if (data.model.kind() == tx::ui::data_kind::table)
        {
            double x = scroll.viewport.x - scroll.x;
            for (const auto column_index : data.model.column_order())
            {
                const auto& column = data.model.columns()[column_index];
                if (!column.visible)
                {
                    continue;
                }
                cell_text(state, painter, {x, y, column.width, data.row_height}, row.id, column.id,
                    row.cells[column_index], column.alignment, available(state) ? theme.foreground : theme.muted);
                x += column.width;
            }
        }
        else
        {
            const double indent = data.model.kind() == tx::ui::data_kind::tree ? 24 + visible.depth * 18 : 0;
            if (data.model.kind() == tx::ui::data_kind::tree)
            {
                tree_marker(state, visible, row, bounds, painter, theme);
            }
            cell_text(state, painter, {bounds.x + indent, bounds.y, std::max(0.0, bounds.width - indent), bounds.height},
                row.id, 0, row.cells[0], 0, available(state) ? theme.foreground : theme.muted);
        }
        if (data.cursor == row.id && root_node(state).focused.lock().get() == &state)
        {
            painter.fill_rect({bounds.x, bounds.y, 2, bounds.height}, theme.accent);
        }
        painter.fill_rect({bounds.x, bounds.y + bounds.height - 1, bounds.width, 1}, theme.border);
    }
    painter.pop_clip();
}
}
