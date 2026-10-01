#include "stdlib/gui/windows/state.hpp"

#include <cmath>

namespace tx_generated::gui
{

double checked_dimension(double value)
{
    if (!std::isfinite(value) || value < 0 || value > 16384)
    {
        fail("invalid_layout", "布局尺寸必须为 0–16384 范围的有限 DIP 值");
    }
    return value;
}

length checked_length(const std::string& mode, double value)
{
    if (mode == "fixed")
    {
        return {length_mode::fixed, checked_dimension(value)};
    }
    if (mode == "auto" && value == 0)
    {
        return {};
    }
    if (mode == "stretch" && std::isfinite(value) && value > 0 && value <= 16384)
    {
        return {length_mode::stretch, value};
    }
    fail("invalid_layout", "length 必须为 fixed、auto(0) 或 stretch(正有限权重，至多 16384)");
}

alignment checked_alignment(const std::string& value)
{
    if (value == "start")
    {
        return alignment::start;
    }
    if (value == "center")
    {
        return alignment::center;
    }
    if (value == "end")
    {
        return alignment::end;
    }
    if (value == "stretch")
    {
        return alignment::stretch;
    }
    fail("invalid_layout", "对齐方式必须为 start/center/end/stretch");
}

void set_layout(node& state, layout_mode mode, double padding, double gap,
    std::int64_t rows, std::int64_t columns)
{
    checked_dimension(padding);
    checked_dimension(gap);
    if (rows < 1 || columns < 1 || rows > static_cast<std::int64_t>(track_limit) ||
        columns > static_cast<std::int64_t>(track_limit))
    {
        fail("invalid_layout", "网格每轴轨道数必须为 1–128");
    }
    if (mode == layout_mode::grid)
    {
        for (const auto& child : state.children)
        {
            if (child->row + child->row_span > static_cast<std::size_t>(rows) ||
                child->column + child->column_span > static_cast<std::size_t>(columns))
            {
                fail("invalid_layout", "现有子控件超出新网格范围");
            }
        }
    }
    std::vector<length> new_rows(rows);
    std::vector<length> new_columns(columns, {length_mode::stretch, 1});
    state.rows.swap(new_rows);
    state.columns.swap(new_columns);
    state.layout = mode;
    state.padding = padding;
    state.gap = gap;
    ++state.revision;
    dirty(state);
}

void set_cell(node& state, std::int64_t row, std::int64_t column,
    std::int64_t row_span, std::int64_t column_span)
{
    const auto parent = state.parent.lock();
    if (!parent || parent->layout != layout_mode::grid || row < 0 || column < 0 ||
        row_span < 1 || column_span < 1 ||
        row >= static_cast<std::int64_t>(parent->rows.size()) ||
        column >= static_cast<std::int64_t>(parent->columns.size()) ||
        row_span > static_cast<std::int64_t>(parent->rows.size()) - row ||
        column_span > static_cast<std::int64_t>(parent->columns.size()) - column)
    {
        fail("invalid_layout", "网格位置或跨度越界，父容器必须先设为 grid");
    }
    state.row = row;
    state.column = column;
    state.row_span = row_span;
    state.column_span = column_span;
    ++state.revision;
    dirty(state);
}

void set_visible(node& state, bool value)
{
    if (state.visible == value)
    {
        return;
    }
    if (!value && (GetFocus() == state.hwnd || IsChild(state.hwnd, GetFocus())))
    {
        advance_focus(root_node(state), &state);
    }
    ShowWindow(state.hwnd, value ? SW_SHOWNA : SW_HIDE);
    state.visible = value;
    ++state.revision;
    dirty(state);
}

void set_enabled(node& state, bool value)
{
    if (state.enabled == value)
    {
        return;
    }
    if (!value && (GetFocus() == state.hwnd || IsChild(state.hwnd, GetFocus())))
    {
        advance_focus(root_node(state), &state);
    }
    EnableWindow(state.hwnd, value);
    state.enabled = value;
    ++state.revision;
}

} // namespace tx_generated::gui
