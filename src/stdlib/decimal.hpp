#pragma once

#include "stdlib/decimal_big.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace tx_generated::decimal_math
{

struct value
{
    bool negative = false;
    natural digits;
    int scale = 0;
};

[[nodiscard]] value parse(std::string_view text);
[[nodiscard]] value from_integer(std::int64_t number);
[[nodiscard]] std::string to_text(const value& number);
[[nodiscard]] value quantize(value number, int scale, std::string_view mode);
[[nodiscard]] value add(const value& left, const value& right,
                        int scale, std::string_view mode);
[[nodiscard]] value subtract(const value& left, const value& right,
                             int scale, std::string_view mode);
[[nodiscard]] value multiply(const value& left, const value& right,
                             int scale, std::string_view mode);
[[nodiscard]] value divide(const value& left, const value& right,
                           int scale, std::string_view mode);
[[nodiscard]] int compare(const value& left, const value& right);

} // namespace tx_generated::decimal_math
