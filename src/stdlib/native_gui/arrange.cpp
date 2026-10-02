#include "stdlib/native_gui/layout_internal.hpp"

#include <algorithm>

namespace tx_generated::native_gui
{
namespace
{
double aligned_offset(gui::alignment alignment, double remaining)
{
    remaining = std::max(0.0, remaining);
    return alignment == gui::alignment::end ? remaining : alignment == gui::alignment::center ? remaining / 2 : 0;
}

double cross_size(gui::length policy, double natural, double available, double minimum,
    double maximum, gui::alignment alignment)
{
    return gui::preferred({policy,
        policy.mode == gui::length_mode::stretch || alignment == gui::alignment::stretch ? available : natural,
        minimum, maximum});
}

rectangle child_bounds(node& child, rectangle slot, gui::extent natural)
{
    slot.x += child.margin;
    slot.y += child.margin;
    slot.width = std::max(0.0, slot.width - 2 * child.margin);
    slot.height = std::max(0.0, slot.height - 2 * child.margin);
    const double width = cross_size(child.width, natural.width, slot.width,
        child.min_width, child.max_width, child.horizontal);
    const double height = cross_size(child.height, natural.height, slot.height,
        child.min_height, child.max_height, child.vertical);
    return {slot.x + aligned_offset(child.horizontal, slot.width - width),
        slot.y + aligned_offset(child.vertical, slot.height - height), width, height};
}

void arrange_axis(node& state, rectangle content)
{
    const bool horizontal = state.layout == layout_mode::row;
    std::vector<std::shared_ptr<node>> children;
    std::vector<gui::extent> natural;
    std::vector<gui::axis_item> items;
    for (const auto& child : state.children)
    {
        if (!child->visible)
        {
            child->clip = {};
            continue;
        }
        children.push_back(child);
        const auto size = measure(*child, std::max(0.0, content.width - 2 * child->margin));
        natural.push_back(size);
        auto policy = horizontal ? child->width : child->height;
        if (policy.mode == gui::length_mode::fixed)
        {
            policy.value += 2 * child->margin;
        }
        items.push_back({policy, (horizontal ? size.width : size.height) + 2 * child->margin,
            (horizontal ? child->min_width : child->min_height) + 2 * child->margin,
            (horizontal ? child->max_width : child->max_height) + 2 * child->margin});
    }
    const double gaps = children.empty() ? 0 : (children.size() - 1) * state.gap;
    const auto sizes = gui::allocate_axis(items, (horizontal ? content.width : content.height) - gaps);
    double cursor = horizontal ? content.x : content.y;
    for (std::size_t index = 0; index < children.size(); ++index)
    {
        auto& child = *children[index];
        const rectangle slot{horizontal ? cursor : content.x, horizontal ? content.y : cursor,
            horizontal ? sizes[index] : content.width, horizontal ? content.height : sizes[index]};
        auto bounds = child_bounds(child, slot, natural[index]);
        // 主轴已经按权重和约束完成分配，不能再被交叉轴对齐规则缩回自然尺寸。
        if (horizontal)
        {
            bounds.x = slot.x + child.margin;
            bounds.width = std::max(0.0, slot.width - 2 * child.margin);
        }
        else
        {
            bounds.y = slot.y + child.margin;
            bounds.height = std::max(0.0, slot.height - 2 * child.margin);
        }
        arrange(child, bounds, state.clip);
        cursor += sizes[index] + state.gap;
    }
}
}

void arrange(node& state, rectangle bounds, rectangle clip)
{
    state.bounds = bounds;
    state.clip = tx::ui::intersect(bounds, clip);
    if (state.kind != tx::graphics_kind::native_panel)
    {
        ensure_text(state, bounds.width - text_inset(state));
        if (state.editor)
        {
            update_editor_viewport(state);
        }
        return;
    }
    const rectangle content{bounds.x + state.padding, bounds.y + state.padding,
        std::max(0.0, bounds.width - 2 * state.padding), std::max(0.0, bounds.height - 2 * state.padding)};
    if (state.layout == layout_mode::grid)
    {
        arrange_grid(state);
    }
    else if (state.layout == layout_mode::overlay)
    {
        for (const auto& child : state.children)
        {
            if (child->visible)
            {
                arrange(*child, child_bounds(*child, content, measure(*child, content.width)), state.clip);
            }
            else
            {
                child->clip = {};
            }
        }
    }
    else
    {
        arrange_axis(state, content);
    }
}
}
