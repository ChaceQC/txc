#pragma once

#include "stdlib/vector.hpp"

#include <string_view>

namespace tx_generated
{

byte_value file_read_bytes(std::string_view path);
void file_write_bytes(std::string_view path, const byte_value& data);
void file_atomic_write_bytes(std::string_view path, const byte_value& data);
void file_atomic_write_text(std::string_view path, std::string_view text,
                            std::string_view encoding);

} // namespace tx_generated
