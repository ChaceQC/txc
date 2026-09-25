#pragma once

#include "stdlib/vector.hpp"

#include <any>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace tx_generated
{

byte_value make_bytes(std::vector<std::uint8_t> data);
const byte_value& bytes_of(const std::any& value);
std::int64_t bytes_length(const byte_value& value);
std::int64_t bytes_at(const byte_value& value, std::int64_t index);
byte_value bytes_from_vector(const int_vector& values);
int_vector bytes_to_vector(const byte_value& value);
byte_value bytes_concat(const byte_value& left, const byte_value& right);
byte_value bytes_slice(const byte_value& value, std::int64_t start,
                       std::int64_t end);
std::string bytes_to_hex(const byte_value& value);
byte_value bytes_from_hex(std::string_view text);
std::string bytes_to_base64(const byte_value& value);
byte_value bytes_from_base64(std::string_view text);

} // namespace tx_generated
