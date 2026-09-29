#pragma once

#include "stdlib/error.hpp"

#include <cstdint>
#include <string_view>

namespace tx_generated
{

enum class parse_error : std::int64_t
{
    none,
    invalid_base,
    empty_int,
    int_sign,
    int_syntax,
    int_range,
    empty_float,
    float_sign,
    float_syntax,
    float_range,
    non_finite
};

template<class value_type>
struct scalar_parse_result
{
    value_type value{};
    parse_error error = parse_error::none;
};

[[nodiscard]] error_info materialize_parse_error(parse_error error);
[[nodiscard]] scalar_parse_result<std::int64_t> parse_int_scalar(
    std::string_view text, std::int64_t base = 10) noexcept;
[[nodiscard]] scalar_parse_result<double> parse_float_scalar(std::string_view text) noexcept;

[[nodiscard]] operation_result<std::int64_t> try_parse_int(
    std::string_view text, std::int64_t base = 10);
[[nodiscard]] operation_result<double> try_parse_float(std::string_view text);

} // namespace tx_generated
