#include "stdlib/native_gui/containers.hpp"
#include "stdlib/native_gui/layout_internal.hpp"

#include <algorithm>

namespace tx_generated::native_gui
{
namespace
{
constexpr double bar_size = 14, header_height = 36, split_width = 8;

tx::ui::rect thumb(tx::ui::rect track, double offset, double extent, double viewport, bool horizontal)
{
    const double length = horizontal ? track.width : track.height;
    if (length <= 0 || extent <= viewport)
    {
        return {};
    }
    const double size = std::min(length, std::max(24.0, length * viewport / extent));
    const double start = offset / (extent - viewport) * (length - size);
    return horizontal ? tx::ui::rect{track.x + start, track.y + 2, size, std::max(0.0, track.height - 4)} :
        tx::ui::rect{track.x + 2, track.y + start, std::max(0.0, track.width - 4), size};
}

void arrange_scroll(node& state)
{
    auto& scroll = *state.scroll;
    if (state.children.empty())
    {
        update_scroll_geometry(scroll, state.bounds, state.bounds.width, state.bounds.height);
        return;
    }
    auto& content = *state.children.front();
    if (!content.visible)
    {
        update_scroll_geometry(scroll, state.bounds, state.bounds.width, state.bounds.height);
        content.clip = {};
        return;
    }
    auto viewport = state.bounds;
    // 滚动条占位会改变文字换行，两方向最多互相触发一次。
    for (unsigned pass = 0; pass < 3; ++pass)
    {
        const auto size = measure(content, scroll.horizontal ? 16384 : viewport.width);
        update_scroll_geometry(scroll, state.bounds,
            scroll.horizontal ? std::max(viewport.width, size.width) : viewport.width,
            scroll.vertical ? std::max(viewport.height, size.height) : viewport.height);
        viewport = scroll.viewport;
    }
    arrange(content, {viewport.x - scroll.x, viewport.y - scroll.y, scroll.width, scroll.height},
        tx::ui::intersect(state.clip, viewport));
}

void arrange_tabs(node& state)
{
    auto& tabs = *state.tabs;
    tabs.headers.clear();
    double x = 0;
    tx::ui::rect selected;
    for (const auto& page : state.children)
    {
        if (!page->visible)
        {
            page->clip = {};
            continue;
        }
        ensure_text(*page, 208);
        const double width = std::clamp(page->text_layout->width() + 32, 80.0, 240.0);
        tabs.headers.push_back({page, {x, 0, width, header_height}});
        if (page->id == tabs.selected)
        {
            selected = {x, 0, width, header_height};
        }
        x += width;
    }
    tabs.width = x;
    const bool reveal = tabs.reveal_selected || tabs.viewport_width != state.bounds.width;
    if (reveal && selected.x < tabs.offset)
    {
        tabs.offset = selected.x;
    }
    else if (reveal && selected.x + selected.width > tabs.offset + state.bounds.width)
    {
        tabs.offset = selected.x + selected.width - state.bounds.width;
    }
    tabs.offset = std::clamp(tabs.offset, 0.0, std::max(0.0, x - state.bounds.width));
    tabs.reveal_selected = false;
    tabs.viewport_width = state.bounds.width;
    for (auto& [page, bounds] : tabs.headers)
    {
        bounds.x += state.bounds.x - tabs.offset;
        bounds.y = state.bounds.y;
    }
    const rectangle content{state.bounds.x, state.bounds.y + header_height, state.bounds.width,
        std::max(0.0, state.bounds.height - header_height)};
    for (const auto& page : state.children)
    {
        if (page->visible && page->layout_visible)
        {
            arrange(*page, content, tx::ui::intersect(state.clip, content));
        }
        else
        {
            page->clip = {};
        }
    }
}

void arrange_split(node& state)
{
    auto& split = *state.split;
    const auto bounds = state.bounds;
    const double span = split.horizontal ? bounds.width : bounds.height;
    const double gap = std::min(span, split_width), available = span - gap;
    double first_min = 0, second_min = 0;
    const auto first_panel = split.panels[0].lock(), second_panel = split.panels[1].lock();
    if (first_panel && !first_panel->closed)
    {
        first_min = split.horizontal ? first_panel->min_width : first_panel->min_height;
    }
    if (second_panel && !second_panel->closed)
    {
        second_min = split.horizontal ? second_panel->min_width : second_panel->min_height;
    }
    const double minimum = std::min(first_min, available);
    const double maximum = std::max(minimum, available - second_min);
    const double first = std::clamp(available * split.ratio, minimum, maximum);
    split.ratio = available > 0 ? first / available : split.ratio;
    split.handle = split.horizontal ? rectangle{bounds.x + first, bounds.y, gap, bounds.height} :
        rectangle{bounds.x, bounds.y + first, bounds.width, gap};
    for (std::size_t index = 0; index < 2; ++index)
    {
        const auto panel = split.panels[index].lock();
        if (!panel || panel->closed)
        {
            continue;
        }
        const double offset = index == 0 ? 0 : first + gap;
        const double length = index == 0 ? first : available - first;
        const rectangle slot = split.horizontal ? rectangle{bounds.x + offset, bounds.y, length, bounds.height} :
            rectangle{bounds.x, bounds.y + offset, bounds.width, length};
        arrange(*panel, slot, tx::ui::intersect(state.clip, slot));
    }
}
}

void update_scroll_geometry(scroll_state& state, tx::ui::rect bounds, double width, double height)
{
    bool horizontal = false, vertical = false;
    for (unsigned pass = 0; pass < 3; ++pass)
    {
        horizontal = state.horizontal && width > std::max(0.0, bounds.width - (vertical ? bar_size : 0));
        vertical = state.vertical && height > std::max(0.0, bounds.height - (horizontal ? bar_size : 0));
    }
    state.viewport = {bounds.x, bounds.y, std::max(0.0, bounds.width - (vertical ? bar_size : 0)),
        std::max(0.0, bounds.height - (horizontal ? bar_size : 0))};
    state.width = state.horizontal ? std::max(width, state.viewport.width) : state.viewport.width;
    state.height = state.vertical ? std::max(height, state.viewport.height) : state.viewport.height;
    state.x = std::clamp(state.x, 0.0, std::max(0.0, state.width - state.viewport.width));
    state.y = std::clamp(state.y, 0.0, std::max(0.0, state.height - state.viewport.height));
    state.horizontal_bar = horizontal ? rectangle{bounds.x, bounds.y + state.viewport.height,
        state.viewport.width, std::min(bar_size, bounds.height)} : rectangle{};
    state.vertical_bar = vertical ? rectangle{bounds.x + state.viewport.width, bounds.y,
        std::min(bar_size, bounds.width), state.viewport.height} : rectangle{};
    state.horizontal_thumb = thumb(state.horizontal_bar, state.x, state.width, state.viewport.width, true);
    state.vertical_thumb = thumb(state.vertical_bar, state.y, state.height, state.viewport.height, false);
}

void arrange_container(node& state)
{
    if (state.tabs)
    {
        arrange_tabs(state);
    }
    else if (state.split)
    {
        arrange_split(state);
    }
    else if (state.scroll)
    {
        arrange_scroll(state);
    }
}
}
