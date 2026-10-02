#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace tx::ui
{
struct unicode_text
{
    std::u32string scalars;
    // 包含终点，供 UTF-8 外部 API 与内部 Unicode 标量索引之间精确转换。
    std::vector<std::size_t> byte_offsets;
};

unicode_text decode_utf8(std::string_view text);
std::string encode_utf8(std::u32string_view text);
std::vector<std::size_t> grapheme_boundaries(std::u32string_view text);
std::vector<std::size_t> word_boundaries(std::u32string_view text);
bool unicode_whitespace(char32_t scalar) noexcept;
}
