#pragma once

#include "backend/cpp/native_gui_result.hpp"
#include "backend/cpp/native_gui_extended_abi.hpp"
#include "backend/cpp/vector_abi_internal.hpp"

namespace tx_generated::native_gui
{
template<tx::ui::data_kind kind>
std::vector<tx::ui::data_row> read_data_rows(const void* value)
{
    const auto& source = detail::vector_value<std::any>(value).data().values;
    if (source.size() > 100000)
    {
        fail("resource_limit", "输入数据超过 100000 行");
    }
    std::vector<tx::ui::data_row> rows;
    rows.reserve(source.size());
    std::size_t scalars = 0;
    for (const auto& value : source)
    {
        const auto& record = std::any_cast<const dynamic_struct&>(value);
        tx::ui::data_row row;
        row.id = std::any_cast<std::int64_t>(record->read_field(0));
        if constexpr (kind == tx::ui::data_kind::table)
        {
            const auto cells = std::any_cast<string_vector>(record->read_field(1));
            if (cells.data().values.size() > 256)
            {
                fail("resource_limit", "表格输入超过 256 列");
            }
            for (const auto& cell : cells.data().values)
            {
                row.cells.push_back(tx::ui::decode_utf8(cell.get()).scalars);
            }
        }
        else if constexpr (kind == tx::ui::data_kind::tree)
        {
            row.parent = std::any_cast<std::int64_t>(record->read_field(1));
            row.cells.push_back(tx::ui::decode_utf8(std::any_cast<std::string>(record->read_field(2))).scalars);
            row.has_children = std::any_cast<bool>(record->read_field(3));
        }
        else
        {
            row.cells.push_back(tx::ui::decode_utf8(std::any_cast<std::string>(record->read_field(1))).scalars);
        }
        for (const auto& cell : row.cells)
        {
            scalars += cell.size();
            if (scalars > 16 * 1024 * 1024 || cell.size() > 65536)
            {
                fail("resource_limit", "输入文本超过模型或单元格大小上限");
            }
        }
        rows.push_back(std::move(row));
    }
    return rows;
}

inline const std::vector<std::int64_t>& read_data_ids(const void* value)
{
    const auto& ids = detail::vector_value<std::int64_t>(value).data().values;
    if (ids.size() > 100000)
    {
        fail("resource_limit", "ID 数量超过 100000");
    }
    return ids;
}

inline void return_data_ids(std::vector<std::int64_t> ids, void** result)
{
    int_vector values;
    values.data().values = std::move(ids);
    values.data().refresh();
    *result = detail::make_handle<std::any>(std::move(values));
}
}
