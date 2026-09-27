#pragma once

#include "stdlib/array.hpp"
#include "stdlib/dictionary.hpp"
#include "stdlib/error.hpp"

#include <any>
#include <cstdint>
#include <string>
#include <string_view>

namespace tx_generated
{

[[nodiscard]] std::any json_parse(std::string_view text);
[[nodiscard]] std::any json_parse_unique(std::string_view text);
[[nodiscard]] tx_dict json_parse_object(std::string_view text);
[[nodiscard]] operation_result<std::any> json_try_parse(std::string_view text);
[[nodiscard]] std::string json_stringify(const std::any& value);
[[nodiscard]] std::string json_stringify_pretty(const std::any& value,
                                                std::int64_t indent);
[[nodiscard]] bool json_contains(const tx_dict& object, std::string_view key);
[[nodiscard]] std::any json_get(const tx_dict& object, std::string_view key);
[[nodiscard]] std::int64_t json_get_int(const tx_dict& object,
                                        std::string_view key);
[[nodiscard]] double json_get_float(const tx_dict& object,
                                    std::string_view key);
[[nodiscard]] bool json_get_bool(const tx_dict& object,
                                 std::string_view key);
[[nodiscard]] std::string json_get_str(const tx_dict& object,
                                       std::string_view key);
[[nodiscard]] tx_array json_get_array(const tx_dict& object,
                                      std::string_view key);
[[nodiscard]] tx_dict json_get_object(const tx_dict& object,
                                      std::string_view key);

} // namespace tx_generated
