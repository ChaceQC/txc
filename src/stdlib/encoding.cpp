#include "stdlib/encoding.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

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

} // namespace

text_encoding parse_encoding(std::string_view name)
{
    std::string normalized;
    normalized.reserve(name.size());
    for (unsigned char character : name)
    {
        if (character == '-' || character == '_')
        {
            continue;
        }
        if (character >= 'A' && character <= 'Z')
        {
            character = static_cast<unsigned char>(character - 'A' + 'a');
        }
        normalized.push_back(static_cast<char>(character));
    }
    if (normalized == "utf8") return text_encoding::utf8;
    if (normalized == "utf8sig") return text_encoding::utf8_sig;
    if (normalized == "utf16") return text_encoding::utf16;
    if (normalized == "utf16le") return text_encoding::utf16le;
    if (normalized == "utf16be") return text_encoding::utf16be;
    if (normalized == "gbk") return text_encoding::gbk;
    if (normalized == "gb18030") return text_encoding::gb18030;
    throw std::runtime_error("不支持的字符集：" + std::string(name));
}

std::wstring utf8_to_wide(std::string_view text)
{
    return decode_code_page(text, CP_UTF8);
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
    return std::filesystem::path(utf8_to_wide(text));
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
        (void)utf8_to_wide(bytes);
        return std::string(bytes);
    }
    if (encoding == text_encoding::gbk || encoding == text_encoding::gb18030)
    {
        return wide_to_utf8(decode_code_page(
            bytes, encoding == text_encoding::gbk ? gbk_code_page : gb18030_code_page));
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
    const auto wide = utf8_to_wide(text);
    if (encoding == text_encoding::utf8 || encoding == text_encoding::utf8_sig)
    {
        return encoding == text_encoding::utf8_sig && include_bom
            ? "\xef\xbb\xbf" + std::string(text) : std::string(text);
    }
    if (encoding == text_encoding::gbk || encoding == text_encoding::gb18030)
    {
        return encode_code_page(wide, encoding == text_encoding::gbk
            ? gbk_code_page : gb18030_code_page);
    }
    const bool little_endian = encoding != text_encoding::utf16be;
    return encode_utf16(wide, little_endian,
                        encoding == text_encoding::utf16 && include_bom);
}

} // namespace tx_generated::detail
