#include "stdlib/gui/windows/layout_internal.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace tx_generated::gui
{
namespace
{

double aligned(double space, double size, alignment value)
{
    return value == alignment::center ? (space - size) / 2 :
        value == alignment::end ? space - size : 0;
}

bounds fit(node& child, bounds cell)
{
    cell.x += child.margin;
    cell.y += child.margin;
    cell.width = std::max(0.0, cell.width - 2 * child.margin);
    cell.height = std::max(0.0, cell.height - 2 * child.margin);
    auto width = child_axis(child, true);
    auto height = child_axis(child, false);
    width.minimum -= 2 * child.margin;
    width.maximum -= 2 * child.margin;
    width.natural -= 2 * child.margin;
    height.minimum -= 2 * child.margin;
    height.maximum -= 2 * child.margin;
    height.natural -= 2 * child.margin;
    if (width.policy.mode == length_mode::fixed)
    {
        width.policy.value -= 2 * child.margin;
    }
    if (height.policy.mode == length_mode::fixed)
    {
        height.policy.value -= 2 * child.margin;
    }
    const auto w = width.policy.mode != length_mode::fixed && child.horizontal == alignment::stretch ?
        std::clamp(cell.width, width.minimum, width.maximum) : preferred(width);
    const auto h = height.policy.mode != length_mode::fixed && child.vertical == alignment::stretch ?
        std::clamp(cell.height, height.minimum, height.maximum) : preferred(height);
    // 网格固定轨道不足时，子 HWND 限于格子内，不能覆盖相邻输入框。
    const auto clipped_width = std::min(cell.width, w);
    const auto clipped_height = std::min(cell.height, h);
    return {cell.x + aligned(cell.width, clipped_width, child.horizontal),
        cell.y + aligned(cell.height, clipped_height, child.vertical), clipped_width, clipped_height};
}

std::vector<double> boundaries(const std::vector<double>& sizes, double gap, double padding)
{
    std::vector<double> result{padding};
    for (const auto size : sizes)
    {
        result.push_back(result.back() + size + gap);
    }
    return result;
}

void arrange_children(node& state)
{
    std::vector<node*> children;
    for (const auto& child : state.children)
    {
        if (participates(*child))
        {
            children.push_back(child.get());
        }
    }
    const auto width = std::max(0.0, state.arranged.width - 2 * state.padding);
    const auto height = std::max(0.0, state.arranged.height - 2 * state.padding);
    if (state.layout == layout_mode::grid)
    {
        const auto columns = allocate_axis(grid_axis(state, true),
            width - state.gap * (state.columns.size() - 1));
        const auto rows = allocate_axis(grid_axis(state, false),
            height - state.gap * (state.rows.size() - 1));
        const auto x = boundaries(columns, state.gap, state.padding);
        const auto y = boundaries(rows, state.gap, state.padding);
        for (auto* child : children)
        {
            arrange(*child, fit(*child, {x[child->column], y[child->row],
                x[child->column + child->column_span] - x[child->column] - state.gap,
                y[child->row + child->row_span] - y[child->row] - state.gap}));
        }
        return;
    }
    const bool horizontal = state.layout == layout_mode::row;
    std::vector<axis_item> items;
    for (const auto* child : children)
    {
        items.push_back(child_axis(*child, horizontal));
    }
    const auto sizes = allocate_axis(items, (horizontal ? width : height) -
        state.gap * (children.empty() ? 0 : children.size() - 1));
    double offset = state.padding;
    for (std::size_t index = 0; index < children.size(); ++index)
    {
        const bounds cell = horizontal ? bounds{offset, state.padding, sizes[index], height} :
            bounds{state.padding, offset, width, sizes[index]};
        arrange(*children[index], fit(*children[index], cell));
        offset += sizes[index] + state.gap;
    }
}

void position_children(node& state)
{
    if (state.children.empty())
    {
        return;
    }
    const auto scale = owner_window(state).dpi / 96.0;
    auto batch = BeginDeferWindowPos(static_cast<int>(state.children.size()));
    if (!batch)
    {
        platform_fail(owner_window(state).owner.lock().get(), "开始批量 GUI 布局", GetLastError());
    }
    for (const auto& child : state.children)
    {
        const auto& rect = child->arranged;
        const auto x = static_cast<int>(std::round(rect.x * scale));
        const auto y = static_cast<int>(std::round(rect.y * scale));
        const auto right = static_cast<int>(std::round((rect.x + rect.width) * scale));
        const auto bottom = static_cast<int>(std::round((rect.y + rect.height) * scale));
        batch = DeferWindowPos(batch, child->hwnd, nullptr, x, y, right - x, bottom - y,
            SWP_NOZORDER | SWP_NOACTIVATE);
        if (!batch)
        {
            platform_fail(owner_window(state).owner.lock().get(), "批量 GUI 布局", GetLastError());
        }
    }
    if (!EndDeferWindowPos(batch))
    {
        platform_fail(owner_window(state).owner.lock().get(), "提交 GUI 布局", GetLastError());
    }
    for (const auto& child : state.children)
    {
        position_children(*child);
    }
}

} // namespace

void arrange(node& state, bounds rectangle)
{
    state.arranged = rectangle;
    if (state.kind == tx::graphics_kind::container)
    {
        arrange_children(state);
    }
}

void layout_tree(node& root, double width, double height)
{
    measure(root, width);
    arrange(root, {0, 0, width, height});
    position_children(root);
}

void flush_layout(graphics::window& window)
{
    if (!window.gui_root)
    {
        return;
    }
    auto& root = *window.gui_root;
    require_idle(root);
    if (!root.dirty || root.arranging)
    {
        return;
    }
    root.arranging = true;
    try
    {
        update_font(root);
        if (!SetWindowPos(root.hwnd, nullptr, 0, 0, window.width, window.height,
            SWP_NOZORDER | SWP_NOACTIVATE))
        {
            platform_fail(window.owner.lock().get(), "调整 GUI 根容器", GetLastError());
        }
        layout_tree(root, window.width * 96.0 / window.dpi, window.height * 96.0 / window.dpi);
        root.dirty = false;
    }
    catch (...)
    {
        root.arranging = false;
        throw;
    }
    root.arranging = false;
}

void flush_all(graphics::app& app)
{
    for (const auto& window : app.windows)
    {
        if (!window->closed)
        {
            flush_layout(*window);
        }
    }
}

} // namespace tx_generated::gui
