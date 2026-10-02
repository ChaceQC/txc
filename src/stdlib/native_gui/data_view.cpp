#include "stdlib/native_gui/state.hpp"
#include "stdlib/native_gui/containers.hpp"

#include <algorithm>

namespace tx_generated::native_gui
{
std::shared_ptr<node> create_data_view(node& parent, tx::ui::data_kind kind, bool multiple,
    std::vector<tx::ui::data_column> columns)
{
    auto data = std::make_unique<data_view_state>(kind, std::move(columns));
    data->multiple = multiple;
    if (!data->model.columns().empty())
    {
        data->focus_column = data->model.columns().front().id;
    }
    const auto type = kind == tx::ui::data_kind::list ? tx::graphics_kind::native_list_view :
        kind == tx::ui::data_kind::table ? tx::graphics_kind::native_table_view : tx::graphics_kind::native_tree_view;
    const auto result = create(parent, type, "");
    result->data = std::move(data);
    result->scroll = std::make_unique<scroll_state>();
    result->scroll->horizontal = kind == tx::ui::data_kind::table;
    result->height = {gui::length_mode::stretch, 1};
    result->min_height = 64;
    return result;
}

void refresh_data(node& state)
{
    auto& data = *state.data;
    const auto& snapshot = data.model.snapshot();
    std::erase_if(data.selected, [&](auto id)
    {
        return !snapshot.indices.contains(id);
    });
    std::erase_if(data.expanded, [&](auto id)
    {
        return !snapshot.indices.contains(id);
    });
    if (!snapshot.indices.contains(data.cursor))
    {
        data.cursor = 0;
    }
    if (!snapshot.indices.contains(data.anchor))
    {
        data.anchor = 0;
    }
    data.rebuild = true;
    data.text_cache.clear();
    const auto& columns = data.model.columns();
    const auto focused = std::find_if(columns.begin(), columns.end(), [&](const auto& column)
    {
        return column.id == data.focus_column && column.visible;
    });
    if (focused == columns.end())
    {
        data.focus_column = 0;
        for (const auto index : data.model.column_order())
        {
            if (columns[index].visible)
            {
                data.focus_column = columns[index].id;
                break;
            }
        }
    }
    state.revision = data.model.revision();
    dirty(state);
}

void data_notification(node& state, const char* kind, std::int64_t id, bool value)
{
    event notification{kind};
    notification.source_id = state.id;
    notification.item_id = id;
    notification.state = value;
    notification.number = static_cast<double>(state.data->selected.size());
    notification.revision = state.data->model.revision();
    enqueue(owner_window(state), std::move(notification));
}

bool set_data_selection(node& state, const std::vector<std::int64_t>& ids, bool notify)
{
    auto& data = *state.data;
    if ((!data.multiple && ids.size() > 1) || ids.size() > data.model.snapshot().rows.size())
    {
        fail("invalid_argument", "选择数量超过视图选择模式或行数");
    }
    std::set<std::int64_t> selection;
    for (const auto id : ids)
    {
        if (!data.model.snapshot().indices.contains(id) || !selection.insert(id).second)
        {
            fail("invalid_argument", "选择的行 ID 不存在或重复");
        }
    }
    const bool changed = data.selected != selection;
    data.selected.swap(selection);
    if (!ids.empty())
    {
        data.cursor = ids.back();
        data.anchor = ids.front();
    }
    owner_window(state).repaint = true;
    if (changed && notify)
    {
        data_notification(state, "data_selection_changed", data.cursor);
    }
    return changed;
}

void select_data_row(node& state, std::int64_t id, bool extend, bool toggle, bool notify)
{
    auto& data = *state.data;
    const auto old_anchor = data.anchor;
    std::vector<std::int64_t> ids;
    if (extend && data.multiple && data.positions.contains(data.anchor) && data.positions.contains(id))
    {
        auto start = data.positions.at(data.anchor), end = data.positions.at(id);
        if (start > end)
        {
            std::swap(start, end);
        }
        for (auto index = start; index <= end; ++index)
        {
            ids.push_back(data.model.snapshot().rows[data.visible[index].index].id);
        }
    }
    else if (toggle && data.multiple)
    {
        ids.assign(data.selected.begin(), data.selected.end());
        if (data.selected.contains(id))
        {
            std::erase(ids, id);
        }
        else
        {
            ids.push_back(id);
        }
    }
    else
    {
        ids.push_back(id);
    }
    const bool changed = set_data_selection(state, ids, false);
    data.cursor = id;
    data.anchor = extend && old_anchor ? old_anchor : id;
    if (notify && changed)
    {
        data_notification(state, "data_selection_changed", id);
    }
    reveal_data_cursor(state);
}

void set_expanded(node& state, std::int64_t id, bool expanded, bool notify)
{
    auto& data = *state.data;
    const auto& snapshot = data.model.snapshot();
    if (!snapshot.indices.contains(id))
    {
        fail("invalid_argument", "展开的树行 ID 不存在");
    }
    const bool changed = expanded ? data.expanded.insert(id).second : data.expanded.erase(id) != 0;
    if (!changed)
    {
        return;
    }
    if (!expanded && snapshot.indices.contains(data.cursor))
    {
        auto parent = snapshot.rows[snapshot.indices.at(data.cursor)].parent;
        while (parent)
        {
            if (parent == id)
            {
                data.cursor = id;
                break;
            }
            parent = snapshot.rows[snapshot.indices.at(parent)].parent;
        }
    }
    data.rebuild = true;
    dirty(state);
    if (notify)
    {
        data_notification(state, "tree_expanded", id, expanded);
        if (expanded && snapshot.rows[snapshot.indices.at(id)].has_children && !snapshot.children.contains(id))
        {
            data_notification(state, "expand_requested", id, true);
        }
    }
}

void reveal_data_cursor(node& state)
{
    layout(state);
    const auto& data = *state.data;
    const auto found = data.positions.find(data.cursor);
    if (found == data.positions.end())
    {
        return;
    }
    const double y = found->second * data.row_height;
    auto& scroll = *state.scroll;
    scroll_to(state, scroll.x, y < scroll.y ? y :
        std::max(scroll.y, y + data.row_height - scroll.viewport.height), false);
}

void request_data_sort(node& state, std::int64_t column_id)
{
    auto& data = *state.data;
    data.sort_ascending = data.sort_column != column_id || !data.sort_ascending;
    data.sort_column = data.focus_column = column_id;
    event notification{"sort_requested"};
    notification.source_id = state.id;
    notification.column_id = column_id;
    notification.state = data.sort_ascending;
    notification.revision = data.model.revision();
    enqueue(owner_window(state), std::move(notification));
    owner_window(state).repaint = true;
}
}
