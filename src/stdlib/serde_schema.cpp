#include "stdlib/serde.hpp"

#include <bit>
#include <stdexcept>

namespace tx_generated
{

std::any serde_default_value(const serde_default& value)
{
    switch (static_cast<serde_kind>(value.kind))
    {
    case serde_kind::integer:
        return value.bits;
    case serde_kind::floating:
        return std::bit_cast<double>(value.bits);
    case serde_kind::boolean:
        return value.bits != 0;
    case serde_kind::text:
        return std::string(value.text, value.length);
    default:
        throw std::runtime_error("编译器生成了无效的 serde 默认值");
    }
}

} // namespace tx_generated
