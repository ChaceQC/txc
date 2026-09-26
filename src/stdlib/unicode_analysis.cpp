#include "stdlib/unicode_text.hpp"

#include "stdlib/error.hpp"

#include <unicode/ubrk.h>
#include <unicode/uchar.h>
#include <unicode/ucol.h>
#include <unicode/utf16.h>

#include <memory>

namespace tx_generated::unicode_text
{
namespace
{

using break_handle = std::unique_ptr<UBreakIterator, decltype(&ubrk_close)>;
using collator_handle = std::unique_ptr<UCollator, decltype(&ucol_close)>;

break_handle character_breaks(const utf16_text& text)
{
    UErrorCode status = U_ZERO_ERROR;
    break_handle result(ubrk_open(UBRK_CHARACTER, "root", text.data(),
        static_cast<std::int32_t>(text.size()), &status), &ubrk_close);
    if (U_FAILURE(status) || !result)
    {
        throw runtime_failure({tx::error_kind::parse, "invalid_unicode",
                               "无法建立 Unicode 字素边界"});
    }
    return result;
}

std::int64_t cluster_width(const UChar* text, std::int32_t start,
                           std::int32_t end)
{
    bool visible = false;
    bool wide = false;
    int regional_count = 0;
    for (auto index = start; index < end;)
    {
        UChar32 point = 0;
        U16_NEXT(text, index, end, point);
        const auto category = u_charType(point);
        if (category == U_CONTROL_CHAR || category == U_LINE_SEPARATOR ||
            category == U_PARAGRAPH_SEPARATOR)
        {
            throw runtime_failure({tx::error_kind::parse, "invalid_character",
                                   "显示宽度只接受单行可显示文本"});
        }
        if (u_hasBinaryProperty(point, UCHAR_REGIONAL_INDICATOR))
        {
            ++regional_count;
        }
        if (category == U_NON_SPACING_MARK || category == U_ENCLOSING_MARK ||
            category == U_COMBINING_SPACING_MARK || category == U_FORMAT_CHAR)
        {
            continue;
        }
        visible = true;
        const auto east_asian = u_getIntPropertyValue(point, UCHAR_EAST_ASIAN_WIDTH);
        wide |= east_asian == U_EA_WIDE || east_asian == U_EA_FULLWIDTH ||
            u_hasBinaryProperty(point, UCHAR_EMOJI_PRESENTATION);
    }
    return !visible ? 0 : wide || regional_count >= 2 ? 2 : 1;
}

} // namespace

std::vector<std::int64_t> code_points(std::string_view text)
{
    const auto wide = from_utf8(text);
    std::vector<std::int64_t> result;
    result.reserve(wide.size());
    for (std::int32_t index = 0; index < static_cast<std::int32_t>(wide.size());)
    {
        UChar32 point = 0;
        U16_NEXT(wide.data(), index, static_cast<std::int32_t>(wide.size()), point);
        result.push_back(point);
    }
    return result;
}

std::vector<std::string> graphemes(std::string_view text)
{
    const auto wide = from_utf8(text);
    auto breaks = character_breaks(wide);
    std::vector<std::string> result;
    for (auto start = ubrk_first(breaks.get()), end = ubrk_next(breaks.get());
         end != UBRK_DONE; start = end, end = ubrk_next(breaks.get()))
    {
        result.push_back(to_utf8(wide.data() + start, end - start));
    }
    return result;
}

std::int64_t grapheme_count(std::string_view text)
{
    const auto wide = from_utf8(text);
    auto breaks = character_breaks(wide);
    std::int64_t result = 0;
    ubrk_first(breaks.get());
    while (ubrk_next(breaks.get()) != UBRK_DONE)
    {
        ++result;
    }
    return result;
}

std::string category(std::int64_t code_point)
{
    if (code_point < 0 || code_point > 0x10ffff ||
        (code_point >= 0xd800 && code_point <= 0xdfff))
    {
        throw runtime_failure({tx::error_kind::parse, "invalid_code_point",
                               "Unicode 码点不在标量值范围内"});
    }
    const auto* name = u_getPropertyValueName(UCHAR_GENERAL_CATEGORY,
        u_charType(static_cast<UChar32>(code_point)), U_SHORT_PROPERTY_NAME);
    return name ? name : "Cn";
}

std::int64_t display_width(std::string_view text)
{
    const auto wide = from_utf8(text);
    auto breaks = character_breaks(wide);
    std::int64_t result = 0;
    for (auto start = ubrk_first(breaks.get()), end = ubrk_next(breaks.get());
         end != UBRK_DONE; start = end, end = ubrk_next(breaks.get()))
    {
        result += cluster_width(wide.data(), start, end);
    }
    return result;
}

std::int64_t compare(std::string_view left, std::string_view right,
                     std::string_view locale, std::string_view strength)
{
    const auto first = from_utf8(left);
    const auto second = from_utf8(right);
    const auto name = locale_name(locale);
    UCollationStrength selected = UCOL_DEFAULT;
    if (strength == "primary") selected = UCOL_PRIMARY;
    else if (strength == "secondary") selected = UCOL_SECONDARY;
    else if (strength == "tertiary") selected = UCOL_TERTIARY;
    else if (strength == "identical") selected = UCOL_IDENTICAL;
    else
    {
        throw runtime_failure({tx::error_kind::parse, "invalid_strength",
                               "比较强度只能是 primary、secondary、tertiary 或 identical"});
    }
    UErrorCode status = U_ZERO_ERROR;
    collator_handle collator(ucol_open(name.c_str(), &status), &ucol_close);
    if (U_FAILURE(status) || !collator)
    {
        throw runtime_failure({tx::error_kind::parse, "invalid_locale",
                               "ICU 无法建立指定 locale 比较器"});
    }
    ucol_setStrength(collator.get(), selected);
    ucol_setAttribute(collator.get(), UCOL_NORMALIZATION_MODE, UCOL_ON, &status);
    if (U_FAILURE(status))
    {
        throw runtime_failure({tx::error_kind::parse, "invalid_locale",
                               "ICU 无法启用规范化比较"});
    }
    const auto order = ucol_strcoll(collator.get(), first.data(),
        static_cast<std::int32_t>(first.size()), second.data(),
        static_cast<std::int32_t>(second.size()));
    return order == UCOL_LESS ? -1 : order == UCOL_GREATER ? 1 : 0;
}

} // namespace tx_generated::unicode_text
