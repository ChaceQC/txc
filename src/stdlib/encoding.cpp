#include "stdlib/encoding.hpp"
#include "common/utf8.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifdef _WIN32
#include <windows.h>
#else
#include <unicode/ucnv.h>
#include <unicode/ucnv_err.h>
#include <memory>
#include <vector>
using UINT = unsigned;
constexpr UINT CP_UTF8 = 65001;
#endif

namespace tx_generated::detail
{
namespace
{

constexpr UINT gbk_code_page = 936;
constexpr UINT gb18030_code_page = 54936;

int checked_length(std::size_t size)
{
    if (size > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    {
        throw std::runtime_error("文本过长，无法进行字符集转换");
    }
    return static_cast<int>(size);
}

#ifdef _WIN32
std::wstring decode_code_page(std::string_view bytes, UINT code_page)
{
    if (bytes.empty())
    {
        return {};
    }
    const DWORD flags = code_page == gbk_code_page ? 0 : MB_ERR_INVALID_CHARS;
    const int length = checked_length(bytes.size());
    const int size = MultiByteToWideChar(code_page, flags, bytes.data(), length,
                                         nullptr, 0);
    if (size == 0)
    {
        throw std::runtime_error("文本包含无效的字符集字节序列");
    }
    std::wstring result(static_cast<std::size_t>(size), L'\0');
    if (MultiByteToWideChar(code_page, flags, bytes.data(), length,
                            result.data(), size) == 0)
    {
        throw std::runtime_error("文本字符集解码失败");
    }
    // Windows 的 GBK 解码不能启用 MB_ERR_INVALID_CHARS，检查替代字符。
    if (code_page == gbk_code_page && result.find(L'\ufffd') != std::wstring::npos)
    {
        throw std::runtime_error("文本包含无效的 GBK 字节序列");
    }
    return result;
}

std::string encode_code_page(std::wstring_view text, UINT code_page)
{
    if (text.empty())
    {
        return {};
    }
    const bool is_gbk = code_page == gbk_code_page;
    const DWORD flags = is_gbk ? WC_NO_BEST_FIT_CHARS :
                        code_page == CP_UTF8 ? WC_ERR_INVALID_CHARS : 0;
    BOOL used_default = FALSE;
    BOOL* default_flag = is_gbk ? &used_default : nullptr;
    const int length = checked_length(text.size());
    const int size = WideCharToMultiByte(code_page, flags, text.data(), length,
                                         nullptr, 0, nullptr, default_flag);
    if (size == 0 || used_default)
    {
        throw std::runtime_error("目标字符集无法表示文本");
    }
    std::string result(static_cast<std::size_t>(size), '\0');
    used_default = FALSE;
    if (WideCharToMultiByte(code_page, flags, text.data(), length,
                            result.data(), size, nullptr, default_flag) == 0 ||
        used_default)
    {
        throw std::runtime_error("目标字符集无法表示文本");
    }
    return result;
}


#else
using converter_handle = std::unique_ptr<UConverter, decltype(&ucnv_close)>;

converter_handle open_converter(UINT code_page)
{
    UErrorCode error = U_ZERO_ERROR;
    converter_handle converter(ucnv_open(code_page == CP_UTF8 ? "UTF-8" :
        code_page == gbk_code_page ? "GBK" : "GB18030", &error), &ucnv_close);
    if (U_SUCCESS(error))
    {
        ucnv_setToUCallBack(converter.get(), UCNV_TO_U_CALLBACK_STOP,
            nullptr, nullptr, nullptr, &error);
        ucnv_setFromUCallBack(converter.get(), UCNV_FROM_U_CALLBACK_STOP,
            nullptr, nullptr, nullptr, &error);
    }
    if (U_FAILURE(error))
    {
        throw std::runtime_error("无法初始化字符集转换");
    }
    return converter;
}

std::wstring decode_code_page(std::string_view bytes, UINT code_page)
{
    auto converter = open_converter(code_page);
    UErrorCode error = U_ZERO_ERROR;
    const auto size = ucnv_toUChars(converter.get(), nullptr, 0, bytes.data(),
        checked_length(bytes.size()), &error);
    if (error != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(error))
    {
        throw std::runtime_error("文本包含无效的字符集字节序列");
    }
    error = U_ZERO_ERROR;
    std::vector<UChar> units(size);
    ucnv_toUChars(converter.get(), units.data(), size, bytes.data(),
        checked_length(bytes.size()), &error);
    if (U_FAILURE(error))
    {
        throw std::runtime_error("文本字符集解码失败");
    }
    // 编码模块内部的 wide 字符串统一存 UTF-16 单元；不传给 Linux 原生路径 API。
    return {units.begin(), units.end()};
}

std::string encode_code_page(std::wstring_view text, UINT code_page)
{
    auto converter = open_converter(code_page);
    std::vector<UChar> units;
    for (const auto unit : text)
    {
        if (static_cast<std::uint32_t>(unit) > 0xffff)
        {
            throw std::runtime_error("UTF-16 单元超出范围");
        }
        units.push_back(static_cast<UChar>(unit));
    }
    UErrorCode error = U_ZERO_ERROR;
    const auto size = ucnv_fromUChars(converter.get(), nullptr, 0, units.data(),
        checked_length(units.size()), &error);
    if (error != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(error))
    {
        throw std::runtime_error("目标字符集无法表示文本");
    }
    error = U_ZERO_ERROR;
    std::string result(size, '\0');
    ucnv_fromUChars(converter.get(), result.data(), size, units.data(),
        checked_length(units.size()), &error);
    if (U_FAILURE(error))
    {
        throw std::runtime_error("目标字符集无法表示文本");
    }
    return result;
}
#endif

bool has_prefix(std::string_view bytes, std::string_view prefix)
{
    return bytes.size() >= prefix.size() && bytes.substr(0, prefix.size()) == prefix;
}

std::string decode_utf16(std::string_view bytes, bool little_endian)
{
    if (bytes.size() % 2 != 0)
    {
        throw std::runtime_error("UTF-16 文件的字节数必须为偶数");
    }
    std::wstring wide;
    wide.reserve(bytes.size() / 2);
    for (std::size_t index = 0; index < bytes.size(); index += 2)
    {
        const auto first = static_cast<unsigned char>(bytes[index]);
        const auto second = static_cast<unsigned char>(bytes[index + 1]);
        const unsigned int unit = little_endian
            ? first | (static_cast<unsigned int>(second) << 8)
            : second | (static_cast<unsigned int>(first) << 8);
        wide.push_back(static_cast<wchar_t>(unit));
    }
    return wide_to_utf8(wide);
}

std::string encode_utf16(std::wstring_view text, bool little_endian, bool bom)
{
    std::string result;
    result.reserve(text.size() * 2 + (bom ? 2 : 0));
    if (bom)
    {
        result += little_endian ? "\xff\xfe" : "\xfe\xff";
    }
    for (wchar_t character : text)
    {
        const auto unit = static_cast<unsigned int>(character);
        const char low = static_cast<char>(unit & 0xff);
        const char high = static_cast<char>((unit >> 8) & 0xff);
        result.push_back(little_endian ? low : high);
        result.push_back(little_endian ? high : low);
    }
    return result;
}

std::string decode_utf32(std::string_view bytes, bool little_endian)
{
    if (bytes.size() % 4 != 0)
    {
        throw std::runtime_error("UTF-32 字节数必须为 4 的倍数");
    }
    std::wstring wide;
    wide.reserve(bytes.size() / 2);
    for (std::size_t index = 0; index < bytes.size(); index += 4)
    {
        std::uint32_t point = 0;
        for (int offset = 0; offset < 4; ++offset)
        {
            const auto byte = static_cast<unsigned char>(bytes[index + offset]);
            point |= static_cast<std::uint32_t>(byte) <<
                (little_endian ? offset * 8 : (3 - offset) * 8);
        }
        if (point > 0x10ffff || (point >= 0xd800 && point <= 0xdfff))
        {
            throw std::runtime_error("UTF-32 包含无效 Unicode 标量");
        }
        if (point <= 0xffff)
        {
            wide.push_back(static_cast<wchar_t>(point));
        }
        else
        {
            point -= 0x10000;
            wide.push_back(static_cast<wchar_t>(0xd800 + (point >> 10)));
            wide.push_back(static_cast<wchar_t>(0xdc00 + (point & 0x3ff)));
        }
    }
    return wide_to_utf8(wide);
}

std::string encode_utf32(std::wstring_view text, bool little_endian, bool bom)
{
    std::string result = bom ? std::string("\xff\xfe\x00\x00", 4) : "";
    result.reserve(text.size() * 4 + result.size());
    for (std::size_t index = 0; index < text.size(); ++index)
    {
        std::uint32_t point = static_cast<std::uint16_t>(text[index]);
        if (point >= 0xd800 && point <= 0xdbff)
        {
            if (++index == text.size())
            {
                throw std::runtime_error("UTF-8 文本包含未配对的代理项");
            }
            const auto low = static_cast<std::uint16_t>(text[index]);
            if (low < 0xdc00 || low > 0xdfff)
            {
                throw std::runtime_error("UTF-8 文本包含未配对的代理项");
            }
            point = 0x10000 + ((point - 0xd800) << 10) + low - 0xdc00;
        }
        else if (point >= 0xdc00 && point <= 0xdfff)
        {
            throw std::runtime_error("UTF-8 文本包含未配对的代理项");
        }
        for (int offset = 0; offset < 4; ++offset)
        {
            const auto shift = little_endian ? offset * 8 : (3 - offset) * 8;
            result.push_back(static_cast<char>((point >> shift) & 0xff));
        }
    }
    return result;
}

} // namespace

std::wstring utf8_to_wide(std::string_view text)
{
    return decode_code_page(text, CP_UTF8);
}

void validate_utf8(std::string_view text)
{
    (void)checked_length(text.size());
    // 短输入直接在调用方扫描，避免为几个码点增加批量入口的调用成本。
    if (text.size() < 32)
    {
        for (std::size_t offset = 0; offset < text.size();)
        {
            const auto width = tx::utf8_width(text, offset);
            if (width == 0)
            {
                throw std::runtime_error("文本包含无效的字符集字节序列");
            }
            offset += width;
        }
        return;
    }
    if (!tx::scan_utf8(text).valid)
    {
        throw std::runtime_error("文本包含无效的字符集字节序列");
    }
}

std::string wide_to_utf8(std::wstring_view text)
{
    return encode_code_page(text, CP_UTF8);
}

std::filesystem::path path_from_utf8(std::string_view text)
{
    if (text.empty() || text.find('\0') != std::string_view::npos)
    {
        throw std::runtime_error("文件路径不能为空或包含 NUL 字符");
    }
#ifdef _WIN32
    return std::filesystem::path(utf8_to_wide(text));
#else
    validate_utf8(text);
    return std::filesystem::path(text);
#endif
}

std::string decode_text(std::string_view bytes, text_encoding encoding)
{
    if (encoding == text_encoding::utf8 || encoding == text_encoding::utf8_sig)
    {
        if (encoding == text_encoding::utf8_sig &&
            has_prefix(bytes, "\xef\xbb\xbf"))
        {
            bytes.remove_prefix(3);
        }
        validate_utf8(bytes);
        return std::string(bytes);
    }
    if (encoding == text_encoding::gbk || encoding == text_encoding::gb18030)
    {
        return wide_to_utf8(decode_code_page(
            bytes, encoding == text_encoding::gbk ? gbk_code_page : gb18030_code_page));
    }

    if (encoding == text_encoding::utf32 || encoding == text_encoding::utf32le ||
        encoding == text_encoding::utf32be)
    {
        const bool little_bom = has_prefix(bytes,
            std::string_view("\xff\xfe\x00\x00", 4));
        const bool big_bom = has_prefix(bytes,
            std::string_view("\x00\x00\xfe\xff", 4));
        bool little_endian = encoding != text_encoding::utf32be;
        if (encoding == text_encoding::utf32)
        {
            if (!little_bom && !big_bom)
            {
                throw std::runtime_error("UTF-32 文本缺少 BOM");
            }
            little_endian = little_bom;
        }
        else if ((little_endian && big_bom) || (!little_endian && little_bom))
        {
            throw std::runtime_error("UTF-32 BOM 与指定字节序不一致");
        }
        if (little_bom || big_bom)
        {
            bytes.remove_prefix(4);
        }
        return decode_utf32(bytes, little_endian);
    }

    bool little_endian = encoding != text_encoding::utf16be;
    const bool little_bom = has_prefix(bytes, "\xff\xfe");
    const bool big_bom = has_prefix(bytes, "\xfe\xff");
    if (encoding == text_encoding::utf16)
    {
        if (!little_bom && !big_bom)
        {
            throw std::runtime_error("UTF-16 文件缺少字节序标记 BOM");
        }
        little_endian = little_bom;
    }
    else if ((little_endian && big_bom) || (!little_endian && little_bom))
    {
        throw std::runtime_error("UTF-16 文件的 BOM 与指定字节序不一致");
    }
    if (little_bom || big_bom)
    {
        bytes.remove_prefix(2);
    }
    return decode_utf16(bytes, little_endian);
}

std::string encode_text(std::string_view text, text_encoding encoding,
                        bool include_bom)
{
    if (encoding == text_encoding::utf8 || encoding == text_encoding::utf8_sig)
    {
        validate_utf8(text);
        return encoding == text_encoding::utf8_sig && include_bom
            ? "\xef\xbb\xbf" + std::string(text) : std::string(text);
    }
    const auto wide = utf8_to_wide(text);
    if (encoding == text_encoding::gbk || encoding == text_encoding::gb18030)
    {
        return encode_code_page(wide, encoding == text_encoding::gbk
            ? gbk_code_page : gb18030_code_page);
    }
    if (encoding == text_encoding::utf32 || encoding == text_encoding::utf32le ||
        encoding == text_encoding::utf32be)
    {
        return encode_utf32(wide, encoding != text_encoding::utf32be,
                            encoding == text_encoding::utf32 && include_bom);
    }
    const bool little_endian = encoding != text_encoding::utf16be;
    return encode_utf16(wide, little_endian,
                        encoding == text_encoding::utf16 && include_bom);
}

} // namespace tx_generated::detail
