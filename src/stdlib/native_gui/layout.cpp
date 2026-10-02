#include "stdlib/native_gui/layout_internal.hpp"

#include <algorithm>
#include <numeric>

namespace tx_generated::native_gui
{
bool contains(rectangle bounds, double x, double y)
{
    return bounds.contains({x, y});
}

void ensure_text(node& state, double width)
{
    width = std::clamp(width, 1.0, 16384.0);
    if (state.text_layout && state.text_width == width)
    {
        return;
    }
    auto& root = root_node(state);
    state.text_layout = std::make_unique<tx::ui::text_layout>(*root.font, display_text(state), 16.0, width,
        !state.editor || state.editor->multiline);
    state.text_width = width;
}

double text_inset(const node& state)
{
    return state.kind == tx::graphics_kind::native_check_box || state.kind == tx::graphics_kind::native_radio_button ? 36 :
        state.kind == tx::graphics_kind::native_button || state.editor ? 24 : 0;
}

namespace
{
gui::extent panel_size(node& state, double width)
{
    if (state.layout == layout_mode::grid)
    {
        const auto tracks = grid_tracks(state, width);
        return {std::accumulate(tracks.columns.begin(), tracks.columns.end(), 0.0) +
            (tracks.columns.size() - 1) * state.gap,
            std::accumulate(tracks.rows.begin(), tracks.rows.end(), 0.0) +
            (tracks.rows.size() - 1) * state.gap};
    }
    gui::extent natural;
    std::size_t count = 0;
    for (const auto& child : state.children)
    {
        if (!child->visible)
        {
            continue;
        }
        const auto size = measure(*child, std::max(0.0, width - 2 * state.padding - 2 * child->margin));
        const auto w = size.width + 2 * child->margin, h = size.height + 2 * child->margin;
        if (state.layout == layout_mode::overlay)
        {
            natural.width = std::max(natural.width, w);
            natural.height = std::max(natural.height, h);
        }
        else if (state.layout == layout_mode::row)
        {
            natural.width += w;
            natural.height = std::max(natural.height, h);
        }
        else
        {
            natural.width = std::max(natural.width, w);
            natural.height += h;
        }
        ++count;
    }
    if (count && state.layout != layout_mode::overlay)
    {
        (state.layout == layout_mode::row ? natural.width : natural.height) += (count - 1) * state.gap;
    }
    return natural;
}
}

gui::extent measure(node& state, double available_width)
{
    const auto width = state.width.mode == gui::length_mode::fixed ? state.width.value : available_width;
    gui::extent natural;
    if (state.kind == tx::graphics_kind::native_panel)
    {
        natural = panel_size(state, width);
        natural.width += 2 * state.padding;
        natural.height += 2 * state.padding;
    }
    else
    {
        ensure_text(state, width - text_inset(state));
        natural.width = state.text_layout->width() + text_inset(state);
        natural.height = std::max(state.text_layout->height() + (interactive(state) ? 16 : 0),
            interactive(state) ? 40.0 : 24.0);
        if (state.kind == tx::graphics_kind::native_progress_bar || state.kind == tx::graphics_kind::native_slider)
        {
            natural.width = 160;
        }
    }
    return {gui::preferred({state.width, natural.width, state.min_width, state.max_width}),
        gui::preferred({state.height, natural.height, state.min_height, state.max_height})};
}

void layout(node& state)
{
    auto& root = root_node(state);
    const auto& window = owner_window(root);
    const double width = window.width * 96.0 / window.dpi, height = window.height * 96.0 / window.dpi;
    if (!root.dirty && width == root.layout_width && height == root.layout_height)
    {
        return;
    }
    const rectangle bounds{0, 0, width, height};
    arrange(root, bounds, bounds);
    root.layout_width = width;
    root.layout_height = height;
    root.dirty = false;
}
}
