#include "stdlib/native_gui/containers.hpp"

#include <algorithm>

namespace tx_generated::native_gui
{
namespace
{
std::shared_ptr<node> with_content(node& parent, tx::graphics_kind kind, unsigned count)
{
    const auto result = create(parent, kind, "");
    try
    {
        result->height = {gui::length_mode::stretch, 1};
        result->min_height = 48;
        result->padding = 0;
        for (unsigned index = 0; index < count; ++index)
        {
            create(*result, tx::graphics_kind::native_panel, "");
        }
        return result;
    }
    catch (...)
    {
        close(*result);
        throw;
    }
}
}

bool container(const node& state)
{
    return state.kind == tx::graphics_kind::native_panel || state.scroll || state.tabs || state.split;
}

std::shared_ptr<node> create_scroll(node& parent, bool horizontal, bool vertical)
{
    if (!horizontal && !vertical)
    {
        fail("invalid_argument", "滚动容器至少启用一个方向");
    }
    const auto result = with_content(parent, tx::graphics_kind::native_scroll, 1);
    result->scroll = std::make_unique<scroll_state>();
    result->scroll->horizontal = horizontal;
    result->scroll->vertical = vertical;
    return result;
}

std::shared_ptr<node> create_tabs(node& parent)
{
    const auto result = with_content(parent, tx::graphics_kind::native_tabs, 0);
    result->tabs = std::make_unique<tabs_state>();
    return result;
}

std::shared_ptr<node> add_tab(node& state, const std::string& title)
{
    if (state.children.size() >= 256)
    {
        fail("resource_limit", "页签数量不得超过 256");
    }
    const auto page = create(state, tx::graphics_kind::native_panel, title);
    normalize_tabs(state);
    return page;
}

std::shared_ptr<node> create_split(node& parent, bool horizontal, double ratio)
{
    if (!std::isfinite(ratio) || ratio < 0 || ratio > 1)
    {
        fail("invalid_argument", "分隔比例必须位于 [0,1]");
    }
    const auto result = with_content(parent, tx::graphics_kind::native_split, 2);
    result->split = std::make_unique<split_state>();
    result->split->horizontal = horizontal;
    result->split->ratio = ratio;
    result->split->panels = {result->children[0], result->children[1]};
    return result;
}

std::shared_ptr<node> content_panel(node& state, std::size_t index)
{
    require_idle(state);
    if (state.split)
    {
        const auto panel = index < 2 ? state.split->panels[index].lock() : nullptr;
        if (!panel || panel->closed)
        {
            fail("closed_resource", "分隔器内容面板已经关闭");
        }
        return panel;
    }
    if (index >= state.children.size() || state.children[index]->closed)
    {
        fail("closed_resource", "容器内容面板已经关闭");
    }
    return state.children[index];
}

void normalize_tabs(node& state)
{
    if (!state.tabs)
    {
        return;
    }
    const auto usable = [](const auto& page)
    {
        return page->visible && page->enabled && !page->closed;
    };
    const auto current = std::find_if(state.children.begin(), state.children.end(), [&](const auto& page)
    {
        return page->id == state.tabs->selected && usable(page);
    });
    if (current == state.children.end())
    {
        const auto first = std::find_if(state.children.begin(), state.children.end(), usable);
        const auto previous = state.tabs->selected;
        state.tabs->selected = first == state.children.end() ? 0 : (*first)->id;
        if (previous != state.tabs->selected)
        {
            ++state.revision;
            state.tabs->reveal_selected = true;
        }
    }
    for (const auto& page : state.children)
    {
        page->layout_visible = page->id == state.tabs->selected;
    }
    auto& root = root_node(state);
    if (const auto focused = root.focused.lock(); focused && !available(*focused))
    {
        focus_node(root, available(state) ? state.shared_from_this() : nullptr);
        reset_interaction(root);
    }
    dirty(state);
}

void select_tab(node& state, std::int64_t id, bool notify)
{
    require_idle(state);
    const auto found = std::find_if(state.children.begin(), state.children.end(), [&](const auto& page)
    {
        return page->id == id && page->visible && page->enabled && !page->closed;
    });
    if (found == state.children.end())
    {
        fail("invalid_argument", "页 ID 不存在，或页面已隐藏/禁用");
    }
    if (state.tabs->selected == id)
    {
        return;
    }
    state.tabs->selected = id;
    state.tabs->reveal_selected = true;
    ++state.revision;
    reset_interaction(root_node(state));
    normalize_tabs(state);
    if (notify)
    {
        event changed{"tab_changed"};
        changed.source_id = state.id;
        changed.item_id = id;
        changed.revision = state.revision;
        enqueue(owner_window(state), std::move(changed));
    }
}

void set_split_ratio(node& state, double ratio, bool notify)
{
    require_idle(state);
    if (!std::isfinite(ratio) || ratio < 0 || ratio > 1)
    {
        fail("invalid_argument", "分隔比例必须位于 [0,1]");
    }
    if (state.split->ratio == ratio)
    {
        return;
    }
    state.split->ratio = ratio;
    ++state.revision;
    dirty(state);
    layout(root_node(state));
    if (notify)
    {
        event changed{"split_changed"};
        changed.source_id = state.id;
        changed.number = state.split->ratio;
        changed.revision = state.revision;
        enqueue(owner_window(state), std::move(changed));
    }
}

bool scroll_to(node& state, double x, double y, bool notify)
{
    require_idle(state);
    if (!std::isfinite(x) || !std::isfinite(y))
    {
        fail("invalid_argument", "滚动偏移必须为有限数");
    }
    auto& scroll = *state.scroll;
    x = std::clamp(x, 0.0, std::max(0.0, scroll.width - scroll.viewport.width));
    y = std::clamp(y, 0.0, std::max(0.0, scroll.height - scroll.viewport.height));
    if (x == scroll.x && y == scroll.y)
    {
        return false;
    }
    scroll.x = x;
    scroll.y = y;
    dirty(state);
    layout(root_node(state));
    if (const auto focused = root_node(state).focused.lock(); focused && focused->editor)
    {
        update_editor_viewport(*focused);
    }
    if (notify)
    {
        event changed{"scroll_changed"};
        changed.source_id = state.id;
        changed.x = x;
        changed.y = y;
        enqueue(owner_window(state), std::move(changed));
    }
    return true;
}

void reveal_node(node& state)
{
    for (auto parent = state.parent.lock(); parent; parent = parent->parent.lock())
    {
        if (!parent->scroll)
        {
            continue;
        }
        const auto& scroll = *parent->scroll;
        const auto adjust = [](double start, double size, double view_start, double view_size)
        {
            if (start < view_start)
            {
                return start - view_start;
            }
            return std::max(0.0, start + std::min(size, view_size) - view_start - view_size);
        };
        scroll_to(*parent, scroll.x + adjust(state.bounds.x, state.bounds.width, scroll.viewport.x, scroll.viewport.width),
            scroll.y + adjust(state.bounds.y, state.bounds.height, scroll.viewport.y, scroll.viewport.height), false);
    }
}
}
