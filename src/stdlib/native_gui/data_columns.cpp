#include "stdlib/native_gui/data_model.hpp"

#include <cmath>
#include <stdexcept>

namespace tx::ui
{
std::size_t data_model::column_index(std::int64_t id) const
{
    for (std::size_t index = 0; index < columns_.size(); ++index)
    {
        if (columns_[index].id == id)
        {
            return index;
        }
    }
    throw std::invalid_argument("列 ID 不存在");
}

void data_model::set_column_width(std::int64_t id, double width)
{
    const auto index = column_index(id);
    if (!std::isfinite(width) || width < 24 || width > 16384)
    {
        throw std::invalid_argument("列宽必须位于 [24,16384]");
    }
    touch();
    columns_[index].width = width;
}

void data_model::set_column_visible(std::int64_t id, bool visible)
{
    const auto index = column_index(id);
    touch();
    columns_[index].visible = visible;
}

void data_model::set_column_order(const std::vector<std::int64_t>& ids)
{
    if (ids.size() != columns_.size())
    {
        throw std::invalid_argument("列顺序必须包含全部列 ID");
    }
    std::unordered_set<std::int64_t> seen;
    std::vector<std::size_t> order;
    for (const auto id : ids)
    {
        if (!seen.insert(id).second)
        {
            throw std::invalid_argument("列顺序包含重复 ID");
        }
        order.push_back(column_index(id));
    }
    touch();
    column_order_.swap(order);
}
}
