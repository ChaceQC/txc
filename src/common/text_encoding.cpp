#include "common/text_encoding.hpp"

#include <stdexcept>
#include <string>

namespace tx
{

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
    if (normalized == "utf8")
    {
        return text_encoding::utf8;
    }
    if (normalized == "utf8sig")
    {
        return text_encoding::utf8_sig;
    }
    if (normalized == "utf16")
    {
        return text_encoding::utf16;
    }
    if (normalized == "utf16le")
    {
        return text_encoding::utf16le;
    }
    if (normalized == "utf16be")
    {
        return text_encoding::utf16be;
    }
    if (normalized == "utf32")
    {
        return text_encoding::utf32;
    }
    if (normalized == "utf32le")
    {
        return text_encoding::utf32le;
    }
    if (normalized == "utf32be")
    {
        return text_encoding::utf32be;
    }
    if (normalized == "gbk")
    {
        return text_encoding::gbk;
    }
    if (normalized == "gb18030")
    {
        return text_encoding::gb18030;
    }
    throw std::runtime_error("不支持的字符集：" + std::string(name));
}

} // namespace tx
