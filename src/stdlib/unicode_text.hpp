#pragma once

#include <unicode/utypes.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace tx_generated::unicode_text
{

using utf16_text = std::basic_string<UChar>;

utf16_text from_utf8(std::string_view text);
std::string to_utf8(const UChar* text, std::int32_t length);
std::string locale_name(std::string_view tag);
std::string data_version();
std::string icu_version();
std::string normalize(std::string_view text, std::string_view form);
std::string case_fold(std::string_view text);
std::string change_case(std::string_view text, std::string_view locale,
                        bool uppercase);
std::vector<std::int64_t> code_points(std::string_view text);
std::vector<std::string> graphemes(std::string_view text);
std::int64_t grapheme_count(std::string_view text);
std::string category(std::int64_t code_point);
std::int64_t display_width(std::string_view text);
std::int64_t compare(std::string_view left, std::string_view right,
                     std::string_view locale, std::string_view strength);

} // namespace tx_generated::unicode_text
