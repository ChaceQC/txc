#include "backend/cpp/native_gui_result.hpp"
#include "backend/cpp/native_gui_abi.hpp"

using namespace tx_generated;
using graphics::resource;

namespace
{
void set_length(resource* control, const void* mode, double value, bool horizontal)
{
    auto& state = native_gui::require_node(control);
    const auto policy = native_gui::checked_length(detail::text_value(mode), value);
    (horizontal ? state.width : state.height) = policy;
    native_gui::dirty(state);
}

void constraints(resource* control, double min_width, double min_height, double max_width, double max_height)
{
    auto& state = native_gui::require_node(control);
    for (const auto value : {min_width, min_height, max_width, max_height})
    {
        native_gui::checked_size(value);
    }
    if (min_width > max_width || min_height > max_height)
    {
        native_gui::fail("invalid_layout", "最小尺寸不能大于最大尺寸");
    }
    state.min_width = min_width;
    state.min_height = min_height;
    state.max_width = max_width;
    state.max_height = max_height;
    native_gui::dirty(state);
}

gui::alignment alignment(const void* value)
{
    const auto& name = detail::text_value(value);
    if (name == "start")
    {
        return gui::alignment::start;
    }
    if (name == "center")
    {
        return gui::alignment::center;
    }
    if (name == "end")
    {
        return gui::alignment::end;
    }
    if (name == "stretch")
    {
        return gui::alignment::stretch;
    }
    native_gui::fail("invalid_layout", "对齐方式必须为 start/center/end/stretch");
}

void set_alignment(resource* control, const void* horizontal, const void* vertical)
{
    auto& state = native_gui::require_node(control);
    const auto x = alignment(horizontal), y = alignment(vertical);
    state.horizontal = x;
    state.vertical = y;
    native_gui::dirty(state);
}

void set_cell(resource* control, std::int64_t row, std::int64_t column, std::int64_t row_span, std::int64_t column_span)
{
    auto& state = native_gui::require_node(control);
    const auto parent = state.parent.lock();
    if (!parent || parent->layout != native_gui::layout_mode::grid || row < 0 || column < 0 ||
        row_span < 1 || column_span < 1 || std::uint64_t(row) >= parent->rows.size() ||
        std::uint64_t(column) >= parent->columns.size() || std::uint64_t(row_span) > parent->rows.size() - row ||
        std::uint64_t(column_span) > parent->columns.size() - column)
    {
        native_gui::fail("invalid_layout", "父项不是网格，或网格位置/跨度越界");
    }
    state.row = row;
    state.column = column;
    state.row_span = row_span;
    state.column_span = column_span;
    native_gui::dirty(state);
}
}

#define tx_native_gui_style(kind) \
extern "C" int txrt_native_gui_set_width_##kind(resource* control, const void* mode, double value) noexcept \
{ \
    return native_gui::leaf_call([&] \
    { \
        set_length(control, mode, value, true); \
    }); \
} \
extern "C" int txrt_native_gui_set_height_##kind(resource* control, const void* mode, double value) noexcept \
{ \
    return native_gui::leaf_call([&] \
    { \
        set_length(control, mode, value, false); \
    }); \
} \
extern "C" int txrt_native_gui_set_constraints_##kind(resource* control, double a, double b, double c, double d) noexcept \
{ \
    return native_gui::leaf_call([&] \
    { \
        constraints(control, a, b, c, d); \
    }); \
} \
extern "C" int txrt_native_gui_set_margin_##kind(resource* control, double margin) noexcept \
{ \
    return native_gui::leaf_call([&] \
    { \
        auto& state = native_gui::require_node(control); \
        state.margin = native_gui::checked_size(margin); \
        native_gui::dirty(state); \
    }); \
} \
extern "C" int txrt_native_gui_set_alignment_##kind(resource* control, const void* x, const void* y) noexcept \
{ \
    return native_gui::leaf_call([&] \
    { \
        set_alignment(control, x, y); \
    }); \
} \
extern "C" int txrt_native_gui_set_cell_##kind(resource* control, std::int64_t row, std::int64_t col, std::int64_t rs, std::int64_t cs) noexcept \
{ \
    return native_gui::leaf_call([&] \
    { \
        set_cell(control, row, col, rs, cs); \
    }); \
}

tx_native_gui_style(panel)
tx_native_gui_style(label)
tx_native_gui_style(button)
tx_native_gui_style(check_box)
tx_native_gui_style(text_box)
tx_native_gui_style(progress_bar)
tx_native_gui_style(slider)
tx_native_gui_style(radio_button)
#undef tx_native_gui_style
