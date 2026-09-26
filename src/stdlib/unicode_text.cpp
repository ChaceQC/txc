#include "stdlib/unicode_text.hpp"

#include "stdlib/error.hpp"

#include <unicode/uloc.h>
#include <unicode/unorm2.h>
#include <unicode/ustring.h>
#include <unicode/uversion.h>

#include <limits>
#include <string>

namespace tx_generated::unicode_text
{
namespace
{

std::int32_t checked_length(std::size_t length)
{
    if (length > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
    {
        throw runtime_failure({tx::error_kind::runtime, "size_limit",
                               "Unicode 文本超出 ICU 长度上限"});
    }
    return static_cast<std::int32_t>(length);
}

[[noreturn]] void unicode_error(const char* code, const char* message)
{
    throw runtime_failure({tx::error_kind::parse, code, message});
}

} // namespace

std::string locale_name(std::string_view tag)
{
    if (tag.empty() || tag.find('\0') != std::string_view::npos)
    {
        unicode_error("invalid_locale", "locale 必须是完整的 BCP 47 标签");
    }
    std::string label(tag);
    char locale[ULOC_FULLNAME_CAPACITY];
    std::int32_t parsed = 0;
    UErrorCode status = U_ZERO_ERROR;
    const auto length = uloc_forLanguageTag(label.c_str(), locale,
        ULOC_FULLNAME_CAPACITY, &parsed, &status);
    if (U_FAILURE(status) || parsed != checked_length(tag.size()) ||
        (length == 0 && tag != "und"))
    {
        unicode_error("invalid_locale", "locale 必须是完整的 BCP 47 标签");
    }
    return std::string(locale, static_cast<std::size_t>(length));
}

namespace
{

const UNormalizer2* normalizer(std::string_view form)
{
    UErrorCode status = U_ZERO_ERROR;
    const UNormalizer2* result = nullptr;
    if (form == "NFC")
    {
        result = unorm2_getNFCInstance(&status);
    }
    else if (form == "NFD")
    {
        result = unorm2_getNFDInstance(&status);
    }
    else if (form == "NFKC")
    {
        result = unorm2_getNFKCInstance(&status);
    }
    else if (form == "NFKD")
    {
        result = unorm2_getNFKDInstance(&status);
    }
    else
    {
        unicode_error("invalid_form", "规范化形式只能是 NFC、NFD、NFKC 或 NFKD");
    }
    if (U_FAILURE(status))
    {
        unicode_error("invalid_unicode", "无法载入 ICU 规范化数据");
    }
    return result;
}

} // namespace

utf16_text from_utf8(std::string_view text)
{
    const auto length = checked_length(text.size());
    UErrorCode status = U_ZERO_ERROR;
    std::int32_t required = 0;
    u_strFromUTF8(nullptr, 0, &required, text.data(), length, &status);
    if (status != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(status))
    {
        unicode_error("invalid_unicode", "文本包含无效的 UTF-8 序列");
    }
    utf16_text result(static_cast<std::size_t>(required), 0);
    status = U_ZERO_ERROR;
    u_strFromUTF8(result.data(), required, nullptr, text.data(), length, &status);
    if (U_FAILURE(status))
    {
        unicode_error("invalid_unicode", "文本包含无效的 UTF-8 序列");
    }
    return result;
}

std::string to_utf8(const UChar* text, std::int32_t length)
{
    UErrorCode status = U_ZERO_ERROR;
    std::int32_t required = 0;
    u_strToUTF8(nullptr, 0, &required, text, length, &status);
    if (status != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(status))
    {
        unicode_error("invalid_unicode", "UTF-16 转 UTF-8 失败");
    }
    std::string result(static_cast<std::size_t>(required), '\0');
    status = U_ZERO_ERROR;
    u_strToUTF8(result.data(), required, nullptr, text, length, &status);
    if (U_FAILURE(status))
    {
        unicode_error("invalid_unicode", "UTF-16 转 UTF-8 失败");
    }
    return result;
}

std::string data_version()
{
    UVersionInfo version{};
    char text[U_MAX_VERSION_STRING_LENGTH];
    u_getUnicodeVersion(version);
    u_versionToString(version, text);
    return text;
}

std::string icu_version()
{
    UVersionInfo version{};
    char text[U_MAX_VERSION_STRING_LENGTH];
    u_getVersion(version);
    u_versionToString(version, text);
    return text;
}

std::string normalize(std::string_view text, std::string_view form)
{
    const auto input = from_utf8(text);
    const auto* selected = normalizer(form);
    UErrorCode status = U_ZERO_ERROR;
    const auto required = unorm2_normalize(selected, input.data(),
        checked_length(input.size()), nullptr, 0, &status);
    if (status != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(status))
    {
        unicode_error("invalid_unicode", "Unicode 规范化失败");
    }
    utf16_text result(static_cast<std::size_t>(required), 0);
    status = U_ZERO_ERROR;
    unorm2_normalize(selected, input.data(), checked_length(input.size()),
        result.data(), required, &status);
    if (U_FAILURE(status))
    {
        unicode_error("invalid_unicode", "Unicode 规范化失败");
    }
    return to_utf8(result.data(), required);
}

std::string case_fold(std::string_view text)
{
    const auto input = from_utf8(text);
    UErrorCode status = U_ZERO_ERROR;
    const auto required = u_strFoldCase(nullptr, 0, input.data(),
        checked_length(input.size()), U_FOLD_CASE_DEFAULT, &status);
    if (status != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(status))
    {
        unicode_error("invalid_unicode", "Unicode case fold 失败");
    }
    utf16_text result(static_cast<std::size_t>(required), 0);
    status = U_ZERO_ERROR;
    u_strFoldCase(result.data(), required, input.data(),
        checked_length(input.size()), U_FOLD_CASE_DEFAULT, &status);
    if (U_FAILURE(status))
    {
        unicode_error("invalid_unicode", "Unicode case fold 失败");
    }
    return to_utf8(result.data(), required);
}

std::string change_case(std::string_view text, std::string_view locale,
                        bool uppercase)
{
    const auto input = from_utf8(text);
    const auto name = locale_name(locale);
    const auto transform = uppercase ? u_strToUpper : u_strToLower;
    UErrorCode status = U_ZERO_ERROR;
    const auto required = transform(nullptr, 0, input.data(),
        checked_length(input.size()), name.c_str(), &status);
    if (status != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(status))
    {
        unicode_error("invalid_unicode", "Unicode 大小写转换失败");
    }
    utf16_text result(static_cast<std::size_t>(required), 0);
    status = U_ZERO_ERROR;
    transform(result.data(), required, input.data(),
        checked_length(input.size()), name.c_str(), &status);
    if (U_FAILURE(status))
    {
        unicode_error("invalid_unicode", "Unicode 大小写转换失败");
    }
    return to_utf8(result.data(), required);
}

} // namespace tx_generated::unicode_text
