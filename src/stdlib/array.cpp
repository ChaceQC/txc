#include "stdlib/stdlib.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>

namespace tx_generated
{

tx_array tx_fn_concat(tx_array left, tx_array right)
{
    tx_array result(left.begin(), left.end());
    result.insert(result.end(), right.begin(), right.end());
    return result;
}

tx_array tx_fn_array_slice(tx_array values, tx_int start, tx_int end)
{
    if (start < 0 || end < start ||
        static_cast<std::uint64_t>(end) > values.size())
    {
        throw std::out_of_range("数组切片范围无效");
    }
    const auto first = values.begin() + static_cast<std::ptrdiff_t>(start);
    const auto last = values.begin() + static_cast<std::ptrdiff_t>(end);
    return tx_array(first, last);
}

tx_array tx_fn_reverse(tx_array values)
{
    tx_array result(values.begin(), values.end());
    std::reverse(result.begin(), result.end());
    return result;
}

void tx_fn_array_push_back(tx_array values, const std::any& value)
{
    values.push_back(value);
}

void tx_fn_array_pop_back(tx_array values)
{
    if (values.size() == 0)
    {
        throw std::out_of_range("不能删除空数组的尾元素");
    }
    values.pop_back();
}

void tx_fn_array_insert(tx_array values, tx_int index, const std::any& value)
{
    if (index < 0 || static_cast<std::uint64_t>(index) > values.size())
    {
        throw std::out_of_range("数组插入位置越界");
    }
    values.insert(static_cast<std::size_t>(index), value);
}

void tx_fn_array_erase(tx_array values, tx_int index)
{
    if (index < 0 || static_cast<std::uint64_t>(index) >= values.size())
    {
        throw std::out_of_range("数组删除位置越界");
    }
    values.erase(static_cast<std::size_t>(index));
}

void tx_fn_array_clear(tx_array values)
{
    values.clear();
}

} // namespace tx_generated
