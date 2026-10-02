#include "backend/cpp/native_gui_result.hpp"
#include "backend/cpp/native_gui_abi.hpp"

using namespace tx_generated;
using graphics::resource;

namespace
{
int make_length(const record_type* type, const char* mode, double value, void** result) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::checked_length(mode, value);
        dynamic_struct item{dynamic_struct_data(type)};
        item->write_field(0, std::string(mode));
        item->write_field(1, value);
        *result = detail::make_handle<std::any>(std::move(item));
    });
}

int set_track(resource* panel, std::int64_t index, const void* mode, double value, bool horizontal) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& state = native_gui::require_node(panel);
        auto& tracks = horizontal ? state.columns : state.rows;
        if (state.layout != native_gui::layout_mode::grid || index < 0 || std::uint64_t(index) >= tracks.size())
        {
            native_gui::fail("invalid_layout", "网格轨道索引越界");
        }
        tracks[index] = native_gui::checked_length(detail::text_value(mode), value);
        native_gui::dirty(state);
    });
}
}

extern "C" int txrt_native_gui_fixed(double value, const record_type* type, void** result) noexcept
{
    return make_length(type, "fixed", value, result);
}

extern "C" int txrt_native_gui_auto_length(const record_type* type, void** result) noexcept
{
    return make_length(type, "auto", 0, result);
}

extern "C" int txrt_native_gui_stretch(double weight, const record_type* type, void** result) noexcept
{
    return make_length(type, "stretch", weight, result);
}

extern "C" int txrt_native_gui_set_grid(resource* panel, std::int64_t rows, std::int64_t columns,
    double padding, double gap) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& state = native_gui::require_node(panel);
        native_gui::checked_size(padding);
        native_gui::checked_size(gap);
        if (rows < 1 || columns < 1 || rows > 128 || columns > 128)
        {
            native_gui::fail("invalid_layout", "网格行列数量必须为 1–128");
        }
        for (const auto& child : state.children)
        {
            if (child->row >= std::uint64_t(rows) || child->row_span > std::uint64_t(rows) - child->row ||
                child->column >= std::uint64_t(columns) || child->column_span > std::uint64_t(columns) - child->column)
            {
                native_gui::fail("invalid_layout", "新网格无法容纳已有子项");
            }
        }
        std::vector<gui::length> next_rows(rows), next_columns(columns, {gui::length_mode::stretch, 1});
        state.rows.swap(next_rows);
        state.columns.swap(next_columns);
        state.layout = native_gui::layout_mode::grid;
        state.padding = padding;
        state.gap = gap;
        native_gui::dirty(state);
    });
}

extern "C" int txrt_native_gui_set_grid_row(resource* panel, std::int64_t index,
    const void* mode, double value) noexcept
{
    return set_track(panel, index, mode, value, false);
}

extern "C" int txrt_native_gui_set_grid_column(resource* panel, std::int64_t index,
    const void* mode, double value) noexcept
{
    return set_track(panel, index, mode, value, true);
}

extern "C" int txrt_native_gui_set_overlay(resource* panel, double padding) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& state = native_gui::require_node(panel);
        native_gui::checked_size(padding);
        state.layout = native_gui::layout_mode::overlay;
        state.padding = padding;
        native_gui::dirty(state);
    });
}
