#include "stdlib/native_gui/containers.hpp"

#include <algorithm>

namespace tx_generated::native_gui
{
namespace
{
using tx::ui::event_kind;
using tx::ui::window_event;

void scroll_pointer(node& state, const window_event& input)
{
    auto& scroll = *state.scroll;
    if (input.kind == event_kind::pointer_down)
    {
        const bool horizontal = contains(scroll.horizontal_bar, input.x, input.y);
        const bool vertical = contains(scroll.vertical_bar, input.x, input.y);
        if (!horizontal && !vertical)
        {
            return;
        }
        const auto thumb = horizontal ? scroll.horizontal_thumb : scroll.vertical_thumb;
        const double position = horizontal ? input.x : input.y;
        if (contains(thumb, input.x, input.y))
        {
            scroll.drag_axis = horizontal ? 1 : 2;
            scroll.drag_origin = position;
            scroll.drag_offset = horizontal ? scroll.x : scroll.y;
        }
        else
        {
            const double direction = position < (horizontal ? thumb.x : thumb.y) ? -1 : 1;
            scroll_to(state, scroll.x + (horizontal ? direction * scroll.viewport.width : 0),
                scroll.y + (vertical ? direction * scroll.viewport.height : 0), true);
        }
    }
    else if (scroll.drag_axis != 0)
    {
        const bool horizontal = scroll.drag_axis == 1;
        const double travel = horizontal ? scroll.horizontal_bar.width - scroll.horizontal_thumb.width :
            scroll.vertical_bar.height - scroll.vertical_thumb.height;
        const double range = horizontal ? scroll.width - scroll.viewport.width : scroll.height - scroll.viewport.height;
        if (travel > 0)
        {
            const double value = scroll.drag_offset + ((horizontal ? input.x : input.y) - scroll.drag_origin) * range / travel;
            scroll_to(state, horizontal ? value : scroll.x, horizontal ? scroll.y : value, true);
        }
        if (input.kind == event_kind::pointer_up)
        {
            scroll.drag_axis = 0;
        }
    }
}

bool change_tab(node& state, int direction, bool edge)
{
    std::vector<std::int64_t> ids;
    for (const auto& page : state.children)
    {
        if (page->visible && page->enabled && !page->closed)
        {
            ids.push_back(page->id);
        }
    }
    if (ids.empty())
    {
        return true;
    }
    const auto found = std::find(ids.begin(), ids.end(), state.tabs->selected);
    const auto current = found == ids.end() ? 0 : static_cast<std::size_t>(found - ids.begin());
    const auto index = edge ? (direction < 0 ? 0 : ids.size() - 1) :
        (current + (direction < 0 ? ids.size() - 1 : 1)) % ids.size();
    select_tab(state, ids[index], true);
    return true;
}

void split_pointer(node& state, const window_event& input)
{
    auto& split = *state.split;
    if (input.kind == event_kind::pointer_down)
    {
        split.dragging = contains(split.handle, input.x, input.y);
    }
    else if (split.dragging)
    {
        const double span = split.horizontal ? state.bounds.width - split.handle.width : state.bounds.height - split.handle.height;
        const double offset = split.horizontal ? input.x - state.bounds.x - split.handle.width / 2 :
            input.y - state.bounds.y - split.handle.height / 2;
        if (span > 0)
        {
            set_split_ratio(state, std::clamp(offset / span, 0.0, 1.0), true);
        }
        if (input.kind == event_kind::pointer_up)
        {
            split.dragging = false;
            event committed{"split_committed"};
            committed.source_id = state.id;
            committed.number = split.ratio;
            committed.revision = state.revision;
            enqueue(owner_window(state), std::move(committed));
        }
    }
}
}

bool container_hit(const node& state, double x, double y)
{
    return (state.scroll && (contains(state.scroll->horizontal_bar, x, y) || contains(state.scroll->vertical_bar, x, y))) ||
        (state.tabs && y < state.bounds.y + 36) || (state.split && contains(state.split->handle, x, y));
}

bool container_pointer(node& state, const window_event& input)
{
    if (state.tabs)
    {
        if (input.kind == event_kind::pointer_down)
        {
            for (const auto& [page, bounds] : state.tabs->headers)
            {
                if (contains(bounds, input.x, input.y) && page->enabled)
                {
                    select_tab(state, page->id, true);
                    break;
                }
            }
        }
    }
    else if (state.split)
    {
        split_pointer(state, input);
    }
    else if (state.scroll)
    {
        scroll_pointer(state, input);
    }
    else if (state.canvas)
    {
        canvas_event(state, input);
    }
    else
    {
        return false;
    }
    return true;
}

bool container_key(node& state, const window_event& input)
{
    if (state.canvas)
    {
        canvas_event(state, input);
        return true;
    }
    if (input.kind != event_kind::key_down || input.alt || input.ctrl || input.meta)
    {
        return false;
    }
    const bool backward = input.key == "left" || input.key == "up" || input.key == "page_up" || input.key == "home";
    const bool forward = input.key == "right" || input.key == "down" || input.key == "page_down" || input.key == "end";
    if (!backward && !forward)
    {
        return false;
    }
    const bool edge = input.key == "home" || input.key == "end";
    if (state.tabs)
    {
        return change_tab(state, backward ? -1 : 1, edge);
    }
    if (state.split)
    {
        set_split_ratio(state, edge ? (backward ? 0 : 1) :
            std::clamp(state.split->ratio + (backward ? -1 : 1) * (input.shift ? 0.1 : 0.02), 0.0, 1.0), true);
        return true;
    }
    if (state.scroll)
    {
        const auto& scroll = *state.scroll;
        const bool horizontal = input.key == "left" || input.key == "right" || (input.shift && scroll.horizontal);
        const double distance = input.key.starts_with("page_") ? scroll.viewport.height : 40;
        const double amount = (backward ? -1 : 1) * distance;
        scroll_to(state, edge ? (backward ? 0 : scroll.width) : scroll.x + (horizontal ? amount : 0),
            edge ? (backward ? 0 : scroll.height) : scroll.y + (horizontal ? 0 : amount), true);
        return true;
    }
    return false;
}

bool switch_tab_from_focus(node& root, const window_event& input)
{
    if (input.kind != event_kind::key_down || input.key != "tab" || !input.ctrl || input.alt || input.meta)
    {
        return false;
    }
    for (auto state = root.focused.lock(); state; state = state->parent.lock())
    {
        if (state->tabs)
        {
            return change_tab(*state, input.shift ? -1 : 1, false);
        }
    }
    return false;
}

bool wheel_scroll(node& state, const window_event& input)
{
    for (auto current = state.shared_from_this(); current; current = current->parent.lock())
    {
        if (current->tabs && input.y < current->bounds.y + 36)
        {
            auto& tabs = *current->tabs;
            const auto offset = std::clamp(tabs.offset - input.wheel * 80, 0.0,
                std::max(0.0, tabs.width - current->bounds.width));
            if (offset != tabs.offset)
            {
                tabs.offset = offset;
                dirty(*current);
                return true;
            }
        }
        if (current->editor && !input.shift)
        {
            ensure_text(*current, std::max(1.0, current->bounds.width - 24));
            const auto value = std::clamp(current->text_scroll_y - input.wheel * 48, 0.0,
                std::max(0.0, current->text_layout->height() - std::max(1.0, current->bounds.height - 16)));
            if (value != current->text_scroll_y)
            {
                current->text_scroll_y = value;
                owner_window(state).repaint = true;
                return true;
            }
        }
        if (current->scroll)
        {
            const auto& scroll = *current->scroll;
            const bool horizontal = input.shift || !scroll.vertical;
            if (scroll_to(*current, scroll.x - (horizontal ? input.wheel * 48 : 0),
                scroll.y - (horizontal ? 0 : input.wheel * 48), true))
            {
                return true;
            }
        }
    }
    return false;
}

void cancel_container_interaction(node& state) noexcept
{
    if (state.data)
    {
        state.data->resizing_column = 0;
        state.data->pressed_column = 0;
    }
    if (state.scroll)
    {
        state.scroll->drag_axis = 0;
    }
    if (state.split)
    {
        state.split->dragging = false;
    }
}

void canvas_event(node& state, const window_event& input)
{
    event result;
    if (input.kind == event_kind::pointer_down || input.kind == event_kind::pointer_up || input.kind == event_kind::pointer_moved)
    {
        result.kind = input.kind == event_kind::pointer_down ? "canvas_pointer_down" :
            input.kind == event_kind::pointer_up ? "canvas_pointer_up" : "canvas_pointer_moved";
        result.x = input.x - state.bounds.x;
        result.y = input.y - state.bounds.y;
        result.number = input.button;
    }
    else
    {
        result.kind = input.kind == event_kind::key_down ? "canvas_key_down" : "canvas_key_up";
        result.text = input.key;
    }
    result.source_id = state.id;
    result.modifiers = (input.shift ? 1 : 0) | (input.ctrl ? 2 : 0) | (input.alt ? 4 : 0) | (input.meta ? 8 : 0);
    enqueue(owner_window(state), std::move(result));
}
}
