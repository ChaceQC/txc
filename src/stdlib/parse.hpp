#pragma once

#include "stdlib/error.hpp"

#include <cstdint>
#include <string_view>

namespace tx_generated
{

[[nodiscard]] operation_result<std::int64_t> try_parse_int(
    std::string_view text, std::int64_t base = 10);
[[nodiscard]] operation_result<double> try_parse_float(std::string_view text);

} // namespace tx_generated
