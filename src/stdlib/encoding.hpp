#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace tx_generated::detail
{

enum class text_encoding
{
    utf8,
    utf8_sig,
    utf16,
    utf16le,
    utf16be,
    utf32,
    utf32le,
    utf32be,
    gbk,
    gb18030
};

text_encoding parse_encoding(std::string_view name);
std::wstring utf8_to_wide(std::string_view text);
std::string wide_to_utf8(std::wstring_view text);
std::filesystem::path path_from_utf8(std::string_view text);
std::string decode_text(std::string_view bytes, text_encoding encoding);
std::string encode_text(std::string_view text, text_encoding encoding,
                        bool include_bom = true);

} // namespace tx_generated::detail
