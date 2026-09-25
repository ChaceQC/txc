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

} // namespace tx_generated
