#pragma once

#include "backend/cpp/graphics_result.hpp"
#include "backend/cpp/vector_abi_internal.hpp"
#include "stdlib/gui/windows/models.hpp"

namespace tx_generated::gui
{

template<tx::graphics_kind kind>
std::vector<data_item> read_items(const void* value)
{
    const auto& source = detail::vector_value<std::any>(value).data().values;
    if (source.size() > (kind == tx::graphics_kind::tree_model ? 10000 : 100000))
    {
        fail("resource_limit", "模型输入超过项数限制");
    }
    std::vector<data_item> result;
    std::size_t bytes = 0;
    result.reserve(source.size());
    for (const auto& value : source)
    {
        const auto& record = std::any_cast<const dynamic_struct&>(value);
        data_item item;
        item.id = std::any_cast<std::int64_t>(record->read_field(0));
        if constexpr (kind == tx::graphics_kind::table_model)
        {
            const auto cells = std::any_cast<string_vector>(record->read_field(1));
            if (cells.data().values.size() > 256)
            {
                fail("invalid_argument", "表格最多 256 列");
            }
            for (const auto& cell : cells.data().values)
            {
                item.cells.push_back(wide_text(cell.get()));
            }
        }
        else if constexpr (kind == tx::graphics_kind::tree_model)
        {
            const auto parent = std::any_cast<dynamic_struct>(record->read_field(1));
            if (std::any_cast<bool>(parent->read_field(0)))
            {
                item.parent = std::any_cast<std::int64_t>(parent->read_field(1));
            }
            item.cells.push_back(wide_text(std::any_cast<std::string>(record->read_field(2))));
            item.load_state = std::any_cast<std::string>(record->read_field(3));
        }
        else
        {
            item.cells.push_back(wide_text(std::any_cast<std::string>(record->read_field(1))));
        }
        for (const auto& text : item.cells)
        {
            bytes += text.size() * sizeof(wchar_t);
        }
        if (bytes > 64 * 1024 * 1024)
        {
            fail("resource_limit", "模型文本超过 64 MiB");
        }
        result.push_back(std::move(item));
    }
    return result;
}

inline const std::vector<std::int64_t>& read_ids(const void* value)
{
    const auto& ids = detail::vector_value<std::int64_t>(value).data().values;
    if (ids.size() > 100000)
    {
        fail("resource_limit", "ID 向量超过 100000 项");
    }
    return ids;
}

} // namespace tx_generated::gui
