#include "stdlib/native_gui/state.hpp"
#include "stdlib/native_gui/containers.hpp"

#include <algorithm>

namespace tx_generated::native_gui
{
namespace
{
using tx::ui::event_kind;
using tx::ui::window_event;

bool header_pointer(node& state, const window_event& input)
{
    auto& data = *state.data;
    if (data.resizing_column)
    {
        if (input.kind == event_kind::pointer_moved || input.kind == event_kind::pointer_up)
        {
            const auto id = data.resizing_column;
            data.model.set_column_width(id, std::clamp(data.resize_width + input.x - data.resize_x, 24.0, 16384.0));
            refresh_data(state);
            if (input.kind == event_kind::pointer_up)
            {
                data.resizing_column = 0;
                event changed{"column_width_changed"};
                changed.source_id = state.id;
                changed.column_id = id;
                changed.revision = data.model.revision();
                enqueue(owner_window(state), std::move(changed));
            }
        }
        return true;
    }
    if (data.model.kind() != tx::ui::data_kind::table || input.y >= state.scroll->viewport.y)
    {
        data.pressed_column = 0;
        return false;
    }
    double x = state.bounds.x - state.scroll->x;
    for (auto index : data.model.column_order())
    {
        const auto& column = data.model.columns()[index];
        if (!column.visible)
        {
            continue;
        }
        if (input.x >= x && input.x < x + column.width)
        {
            if (input.kind == event_kind::pointer_down && input.x > x + column.width - 5)
            {
                data.resizing_column = column.id;
                data.resize_x = input.x;
                data.resize_width = column.width;
            }
            else if (input.kind == event_kind::pointer_down)
            {
                data.pressed_column = column.id;
            }
            else if (input.kind == event_kind::pointer_up && data.pressed_column == column.id)
            {
                data.pressed_column = 0;
                request_data_sort(state, column.id);
            }
            return true;
        }
        x += column.width;
    }
    return true;
}

bool tree_key(node& state, const window_event& input)
{
    auto& data = *state.data;
    const auto& snapshot = data.model.snapshot();
    if (data.model.kind() != tx::ui::data_kind::tree || !snapshot.indices.contains(data.cursor) ||
        (input.key != "left" && input.key != "right"))
    {
        return false;
    }
    const auto& row = snapshot.rows[snapshot.indices.at(data.cursor)];
    if (input.key == "left")
    {
        if (data.expanded.contains(row.id))
        {
            set_expanded(state, row.id, false, true);
        }
        else if (row.parent)
        {
            select_data_row(state, row.parent, input.shift, false, true);
        }
    }
    else if (!data.expanded.contains(row.id) && (row.has_children || snapshot.children.contains(row.id)))
    {
        set_expanded(state, row.id, true, true);
    }
    else if (const auto children = snapshot.children.find(row.id); children != snapshot.children.end() && !children->second.empty())
    {
        select_data_row(state, snapshot.rows[children->second.front()].id, input.shift, false, true);
    }
    return true;
}

bool column_key(node& state, const window_event& input)
{
    auto& data = *state.data;
    if (data.model.kind() != tx::ui::data_kind::table)
    {
        return false;
    }
    if (input.ctrl && input.key == "space")
    {
        if (data.focus_column && !input.repeat)
        {
            request_data_sort(state, data.focus_column);
        }
        return true;
    }
    if (input.key != "left" && input.key != "right")
    {
        return false;
    }
    std::vector<const tx::ui::data_column*> columns;
    for (auto index : data.model.column_order())
    {
        const auto& column = data.model.columns()[index];
        if (column.visible)
        {
            columns.push_back(&column);
        }
    }
    if (columns.empty())
    {
        return true;
    }
    auto found = std::find_if(columns.begin(), columns.end(), [&](auto column)
    {
        return column->id == data.focus_column;
    });
    auto index = found == columns.end() ? 0 : static_cast<int>(found - columns.begin());
    index = std::clamp(index + (input.key == "left" ? -1 : 1), 0, static_cast<int>(columns.size() - 1));
    data.focus_column = columns[index]->id;
    double x = 0;
    for (int previous = 0; previous < index; ++previous)
    {
        x += columns[previous]->width;
    }
    scroll_to(state, x < state.scroll->x ? x : std::max(state.scroll->x,
        x + columns[index]->width - state.scroll->viewport.width), state.scroll->y, false);
    owner_window(state).repaint = true;
    return true;
}
}

bool data_pointer(node& state, const window_event& input)
{
    if (!state.data)
    {
        return false;
    }
    auto& data = *state.data;
    if (!data.resizing_column && (state.scroll->drag_axis || container_hit(state, input.x, input.y)))
    {
        return false;
    }
    if (header_pointer(state, input))
    {
        return true;
    }
    if (input.kind != event_kind::pointer_down || !contains(state.scroll->viewport, input.x, input.y))
    {
        return true;
    }
    const auto position = static_cast<std::size_t>((input.y - state.scroll->viewport.y + state.scroll->y) / data.row_height);
    if (position >= data.visible.size())
    {
        if (!input.ctrl && !input.shift)
        {
            set_data_selection(state, {}, true);
        }
        return true;
    }
    const auto visible = data.visible[position];
    const auto& row = data.model.snapshot().rows[visible.index];
    if (data.model.kind() == tx::ui::data_kind::tree && input.x < state.bounds.x + 24 + visible.depth * 18 &&
        (row.has_children || data.model.snapshot().children.contains(row.id)))
    {
        set_expanded(state, row.id, !data.expanded.contains(row.id), true);
    }
    else
    {
        select_data_row(state, row.id, input.shift, input.ctrl, true);
    }
    return true;
}

bool data_key(node& state, const window_event& input)
{
    if (!state.data || input.kind != event_kind::key_down || input.alt || input.meta)
    {
        return false;
    }
    auto& data = *state.data;
    if (column_key(state, input) || tree_key(state, input))
    {
        return true;
    }
    if (input.ctrl && input.key == "a" && data.multiple)
    {
        std::vector<std::int64_t> ids;
        for (const auto& row : data.visible)
        {
            ids.push_back(data.model.snapshot().rows[row.index].id);
        }
        set_data_selection(state, ids, true);
        return true;
    }
    if (input.key == "enter" || input.key == "space")
    {
        if (!input.repeat && data.cursor)
        {
            if (input.key == "space")
            {
                select_data_row(state, data.cursor, input.shift, input.ctrl, true);
            }
            else
            {
                data_notification(state, "data_activated", data.cursor);
            }
        }
        return true;
    }
    const bool backward = input.key == "up" || input.key == "page_up" || input.key == "home";
    const bool forward = input.key == "down" || input.key == "page_down" || input.key == "end";
    if ((!backward && !forward) || data.visible.empty())
    {
        return false;
    }
    const auto found = data.positions.find(data.cursor);
    auto index = found == data.positions.end() ? (backward ? static_cast<int>(data.visible.size()) : -1) :
        static_cast<int>(found->second);
    const auto distance = input.key.starts_with("page_") ? std::max(1, static_cast<int>(state.scroll->viewport.height / data.row_height)) : 1;
    index = input.key == "home" ? 0 : input.key == "end" ? static_cast<int>(data.visible.size()) - 1 :
        std::clamp(index + (backward ? -distance : distance), 0, static_cast<int>(data.visible.size()) - 1);
    const auto id = data.model.snapshot().rows[data.visible[index].index].id;
    if (input.ctrl && !input.shift)
    {
        data.cursor = id;
        reveal_data_cursor(state);
        owner_window(state).repaint = true;
    }
    else
    {
        select_data_row(state, id, input.shift, false, true);
    }
    return true;
}
}
