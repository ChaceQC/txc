#pragma once

#include <cstdint>
#include <string_view>

namespace tx
{

enum class text_encoding : std::int64_t
{
    utf8, utf8_sig, utf16, utf16le, utf16be,
    utf32, utf32le, utf32be, gbk, gb18030
};

text_encoding parse_encoding(std::string_view name);

} // namespace tx
