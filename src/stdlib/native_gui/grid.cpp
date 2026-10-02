#include "stdlib/native_gui/layout_internal.hpp"

#include <algorithm>
#include <numeric>

namespace tx_generated::native_gui
{
namespace
{
void contribute(const std::vector<gui::length>& policies, std::vector<double>& natural,
    std::size_t first, std::size_t span, double required)
{
    if (!span || first >= policies.size() || span > policies.size() - first)
    {
        fail("invalid_layout", "网格子项的轨道或跨度越界");
    }
    double fixed = 0;
    std::size_t flexible = 0;
    for (std::size_t i = first; i < first + span; ++i)
    {
        if (policies[i].mode == gui::length_mode::fixed)
        {
            fixed += policies[i].value;
        }
        else
        {
            ++flexible;
        }
    }
    if (flexible)
    {
        const auto share = std::max(0.0, required - fixed) / flexible;
        for (std::size_t i = first; i < first + span; ++i)
        {
            if (policies[i].mode != gui::length_mode::fixed)
            {
                natural[i] = std::max(natural[i], share);
            }
        }
    }
}

std::vector<double> allocate(const std::vector<gui::length>& policies,
    const std::vector<double>& natural, double available)
{
    std::vector<gui::axis_item> items;
    for (std::size_t index = 0; index < policies.size(); ++index)
    {
        items.push_back({policies[index], natural[index], 0, 16384});
    }
    if (available < 0)
    {
        std::vector<double> sizes;
        for (const auto& item : items)
        {
            sizes.push_back(gui::preferred(item));
        }
        return sizes;
    }
    return gui::allocate_axis(items, available);
}

double span_size(const std::vector<double>& values, std::size_t first, std::size_t count, double gap)
{
    return std::accumulate(values.begin() + first, values.begin() + first + count, 0.0) + (count - 1) * gap;
}
}

grid_geometry grid_tracks(node& state, double width, double height)
{
    if (state.columns.empty() || state.rows.empty())
    {
        fail("invalid_layout", "网格必须具有行与列");
    }
    std::vector<double> columns(state.columns.size()), rows(state.rows.size());
    const double inner_width = std::max(0.0, width - 2 * state.padding);
    for (const auto& child : state.children)
    {
        if (child->visible)
        {
            const auto size = measure(*child, inner_width);
            contribute(state.columns, columns, child->column, child->column_span,
                size.width + 2 * child->margin - (child->column_span - 1) * state.gap);
        }
    }
    columns = allocate(state.columns, columns, std::max(0.0, inner_width - (columns.size() - 1) * state.gap));
    for (const auto& child : state.children)
    {
        if (child->visible)
        {
            const auto width = span_size(columns, child->column, child->column_span, state.gap);
            const auto size = measure(*child, std::max(0.0, width - 2 * child->margin));
            contribute(state.rows, rows, child->row, child->row_span,
                size.height + 2 * child->margin - (child->row_span - 1) * state.gap);
        }
    }
    const double inner_height = height < 0 ? -1 :
        std::max(0.0, height - 2 * state.padding - (rows.size() - 1) * state.gap);
    return {columns, allocate(state.rows, rows, inner_height)};
}

void arrange_grid(node& state)
{
    const auto grid = grid_tracks(state, state.bounds.width, state.bounds.height);
    for (const auto& child : state.children)
    {
        if (!child->visible)
        {
            child->clip = {};
            continue;
        }
        const double x = state.bounds.x + state.padding +
            std::accumulate(grid.columns.begin(), grid.columns.begin() + child->column, 0.0) + child->column * state.gap;
        const double y = state.bounds.y + state.padding +
            std::accumulate(grid.rows.begin(), grid.rows.begin() + child->row, 0.0) + child->row * state.gap;
        const double width = std::max(0.0, span_size(grid.columns, child->column, child->column_span, state.gap) - 2 * child->margin);
        const double height = std::max(0.0, span_size(grid.rows, child->row, child->row_span, state.gap) - 2 * child->margin);
        const auto natural = measure(*child, width);
        const double w = gui::preferred({child->width, child->horizontal == gui::alignment::stretch ? width : natural.width,
            child->min_width, child->max_width});
        const double h = gui::preferred({child->height, child->vertical == gui::alignment::stretch ? height : natural.height,
            child->min_height, child->max_height});
        const auto offset = [](gui::alignment alignment, double remaining)
        {
            return std::max(0.0, remaining) * (alignment == gui::alignment::end ? 1 : alignment == gui::alignment::center ? 0.5 : 0);
        };
        arrange(*child, {x + child->margin + offset(child->horizontal, width - w),
            y + child->margin + offset(child->vertical, height - h), w, h}, state.clip);
    }
}
}
