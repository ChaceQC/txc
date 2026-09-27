#pragma once

#include "common/text_encoding.hpp"

#include <filesystem>
#include <string>
#include <string_view>

namespace tx_generated::detail
{

using tx::text_encoding;
using tx::parse_encoding;
void validate_utf8(std::string_view text);
std::wstring utf8_to_wide(std::string_view text);
std::string wide_to_utf8(std::wstring_view text);
std::filesystem::path path_from_utf8(std::string_view text);
std::string decode_text(std::string_view bytes, text_encoding encoding);
std::string encode_text(std::string_view text, text_encoding encoding,
                        bool include_bom = true);

} // namespace tx_generated::detail
