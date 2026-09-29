#include "driver/child_process.hpp"

namespace tx
{

std::string json_quote(const std::string& value)
{
    constexpr char hex[] = "0123456789abcdef";
    std::string result = "\"";
    for (std::size_t index = 0; index < value.size(); ++index)
    {
        const auto byte = static_cast<unsigned char>(value[index]);
        if (byte == '"' || byte == '\\')
        {
            result += '\\';
            result += static_cast<char>(byte);
        }
        else if (byte < 0x20)
        {
            result += "\\u00";
            result += hex[byte >> 4];
            result += hex[byte & 15];
        }
        else if (byte >= 0x80)
        {
            const std::size_t length = byte >= 0xc2 && byte <= 0xdf ? 2 :
                byte >= 0xe0 && byte <= 0xef ? 3 :
                byte >= 0xf0 && byte <= 0xf4 ? 4 : 0;
            std::uint32_t codepoint = byte & (length == 2 ? 0x1f :
                length == 3 ? 0x0f : 0x07);
            bool valid = length != 0 && index + length <= value.size();
            for (std::size_t part = 1; valid && part < length; ++part)
            {
                const auto continuation = static_cast<unsigned char>(
                    value[index + part]);
                valid = (continuation & 0xc0) == 0x80;
                codepoint = (codepoint << 6) | (continuation & 0x3f);
            }
            valid = valid && codepoint >= (length == 2 ? 0x80u :
                length == 3 ? 0x800u : 0x10000u) &&
                codepoint <= 0x10ffffu &&
                (codepoint < 0xd800u || codepoint > 0xdfffu);
            if (valid)
            {
                result.append(value, index, length);
                index += length - 1;
            }
            else
            {
                result += "\\ufffd";
            }
        }
        else
        {
            result += static_cast<char>(byte);
        }
    }
    return result + '"';
}

} // namespace tx
