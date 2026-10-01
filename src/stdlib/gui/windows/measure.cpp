#define WINVER 0x0A00
#define _WIN32_WINNT 0x0A00
#include "stdlib/gui/windows/layout_internal.hpp"
#include "stdlib/gui/windows/models.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace tx_generated::gui
{

bool participates(const node& state)
{
    return !state.closed && state.page_active && (state.visible || state.reserved);
}

axis_item child_axis(const node& child, bool horizontal)
{
    auto policy = horizontal ? child.width : child.height;
    const auto parent = child.parent.lock();
    if (parent && (horizontal ? parent->scroll_horizontal : parent->scroll_vertical) &&
        policy.mode == length_mode::stretch)
    {
        policy = {};
    }
    if (horizontal && !child.explicit_width && parent && parent->layout == layout_mode::row)
    {
        policy = {};
    }
    const auto margin = child.margin * 2;
    if (policy.mode == length_mode::fixed)
    {
        policy.value += margin;
    }
    return {policy, (horizontal ? child.natural.width : child.natural.height) + margin,
        (horizontal ? child.minimum.width : child.minimum.height) + margin,
        (horizontal ? child.maximum.width : child.maximum.height) + margin};
}

std::vector<axis_item> grid_axis(node& state, bool horizontal)
{
    std::vector<axis_item> items;
    for (const auto& track : horizontal ? state.columns : state.rows)
    {
        const bool unbounded = horizontal ? state.scroll_horizontal : state.scroll_vertical;
        items.push_back({unbounded && track.mode == length_mode::stretch ? length{} : track, 0, 0, 16384});
    }
    // 先单格，再跨度；跨度需求只增加非 fixed 轨道。
    for (std::size_t span = 1; span <= items.size(); ++span)
    {
        for (const auto& child : state.children)
        {
            const auto child_span = horizontal ? child->column_span : child->row_span;
            if (!participates(*child) || span != child_span)
            {
                continue;
            }
            const auto start = horizontal ? child->column : child->row;
            const auto axis = child_axis(*child, horizontal);
            for (const bool minimum : {false, true})
            {
                double covered = state.gap * (span - 1);
                std::size_t flexible = 0;
                for (std::size_t index = start; index < start + span; ++index)
                {
                    covered += minimum ? items[index].minimum : preferred(items[index]);
                    flexible += items[index].policy.mode != length_mode::fixed;
                }
                const auto need = minimum ? axis.minimum : preferred(axis);
                if (!flexible || need <= covered)
                {
                    continue;
                }
                for (std::size_t index = start; index < start + span; ++index)
                {
                    auto& track = items[index];
                    if (track.policy.mode != length_mode::fixed)
                    {
                        auto& target = minimum ? track.minimum : track.natural;
                        target = std::min(track.maximum, target + (need - covered) / flexible);
                    }
                }
            }
        }
    }
    return items;
}

namespace
{

extent text_extent(node& state, double available)
{
    const auto& window = owner_window(state);
    const auto scale = window.dpi / 96.0;
    auto dc = GetDC(state.hwnd);
    if (!dc)
    {
        platform_fail(window.owner.lock().get(), "测量控件文本", GetLastError());
    }
    const auto previous = SelectObject(dc, root_node(state).font);
    const auto text = wide_text(state.text.empty() ? " " : state.text);
    RECT rectangle{0, 0, static_cast<LONG>(std::ceil(std::max(1.0, available) * scale)), 0};
    const UINT flags = DT_CALCRECT | DT_NOPREFIX |
        (state.kind == tx::graphics_kind::label ? DT_WORDBREAK : DT_SINGLELINE);
    DrawTextW(dc, text.c_str(), static_cast<int>(text.size()), &rectangle, flags);
    TEXTMETRICW metric{};
    GetTextMetricsW(dc, &metric);
    SelectObject(dc, previous);
    ReleaseDC(state.hwnd, dc);
    const auto text_height = metric.tmHeight / scale;
    const auto width = rectangle.right / scale;
    const auto height = rectangle.bottom / scale;
    using tx::graphics_kind;
    if (state.toolbar)
    {
        return {160, 32};
    }
    if (state.kind == graphics_kind::progress_bar || state.kind == graphics_kind::slider)
    {
        return {160, 28};
    }
    if (state.kind == graphics_kind::gui_canvas)
    {
        return {240, 160};
    }
    if (state.model)
    {
        return {240, 160};
    }
    if (state.kind == graphics_kind::button)
    {
        return {std::max(75.0, width + 24), std::max(26.0, text_height + 12)};
    }
    if (state.kind == graphics_kind::check_box)
    {
        return {width + 26, std::max(24.0, text_height + 8)};
    }
    if (state.kind == graphics_kind::text_box)
    {
        return {160, text_height * (state.multiline ? 4 : 1) + 12};
    }
    return {width, height};
}

double track_total(const std::vector<axis_item>& tracks, double gap)
{
    double total = gap * (tracks.size() - 1);
    for (const auto& item : tracks)
    {
        total += preferred(item);
    }
    return total;
}

} // namespace

extent measure(node& state, double available_width)
{
    available_width = std::clamp(state.width.mode == length_mode::fixed ?
        state.width.value : available_width, state.minimum.width, state.maximum.width);
    if (state.scroll_vertical)
    {
        const auto dpi = owner_window(state).dpi;
        available_width = std::max(0.0, available_width - GetSystemMetricsForDpi(SM_CXVSCROLL, dpi) * 96.0 / dpi);
    }
    if ((state.kind != tx::graphics_kind::container && state.kind != tx::graphics_kind::tabs) || state.toolbar)
    {
        state.natural = text_extent(state, available_width);
        return state.natural;
    }
    extent result;
    std::size_t count = 0;
    for (const auto& child : state.children)
    {
        if (!participates(*child))
        {
            continue;
        }
        measure(*child, state.scroll_horizontal ? 16384 :
            std::max(0.0, available_width - 2 * (state.padding + child->margin)));
        const auto width = preferred(child_axis(*child, true));
        const auto height = preferred(child_axis(*child, false));
        if (state.layout == layout_mode::row)
        {
            result.width += width;
            result.height = std::max(result.height, height);
        }
        else
        {
            result.width = std::max(result.width, width);
            result.height += height;
        }
        ++count;
    }
    if (state.layout == layout_mode::grid)
    {
        const auto columns = allocate_axis(grid_axis(state, true),
            available_width - 2 * state.padding - state.gap * (state.columns.size() - 1));
        for (const auto& child : state.children)
        {
            if (participates(*child))
            {
                double width = state.gap * (child->column_span - 1);
                for (auto index = child->column; index < child->column + child->column_span; ++index)
                {
                    width += columns[index];
                }
                measure(*child, std::max(0.0, width - 2 * child->margin));
            }
        }
        result.width = track_total(grid_axis(state, true), state.gap);
        result.height = track_total(grid_axis(state, false), state.gap);
    }
    else if (state.layout == layout_mode::row && count && !state.scroll_horizontal)
    {
        std::vector<axis_item> items;
        for (const auto& child : state.children)
        {
            if (participates(*child))
            {
                items.push_back(child_axis(*child, true));
            }
        }
        const auto widths = allocate_axis(items,
            available_width - 2 * state.padding - state.gap * (count - 1));
        std::size_t index = 0;
        result.height = 0;
        for (const auto& child : state.children)
        {
            if (participates(*child))
            {
                measure(*child, std::max(0.0, widths[index++] - 2 * child->margin));
                result.height = std::max(result.height, preferred(child_axis(*child, false)));
            }
        }
        result.width += (count - 1) * state.gap;
    }
    else if (count > 1)
    {
        (state.layout == layout_mode::row ? result.width : result.height) += (count - 1) * state.gap;
    }
    result.width += 2 * state.padding;
    result.height += 2 * state.padding;
    if (state.kind == tx::graphics_kind::tabs)
    {
        result.width += 8;
        result.height += 32;
    }
    state.natural = result;
    return result;
}

void update_font(node& root)
{
    const auto dpi = owner_window(root).dpi;
    if (root.font && root.font_dpi == dpi)
    {
        return;
    }
    NONCLIENTMETRICSW metrics{};
    metrics.cbSize = sizeof(metrics);
    if (!SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0, dpi))
    {
        platform_fail(owner_window(root).owner.lock().get(), "读取系统 GUI 字体", GetLastError());
    }
    const auto font = CreateFontIndirectW(&metrics.lfMessageFont);
    if (!font)
    {
        platform_fail(owner_window(root).owner.lock().get(), "创建 GUI 字体", GetLastError());
    }
    const auto previous = root.font;
    root.font = font;
    root.font_dpi = dpi;
    const auto apply = [&](auto&& self, node& current) -> void
    {
        SendMessageW(current.hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        view_dpi_changed(current);
        for (const auto& child : current.children)
        {
            self(self, *child);
        }
    };
    apply(apply, root);
    if (previous)
    {
        DeleteObject(previous);
    }
}

} // namespace tx_generated::gui
