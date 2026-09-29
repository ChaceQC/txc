#include "serde_payload.hpp"

#include <charconv>
#include <cstdint>
#include <stdexcept>

namespace performance_equivalence
{
namespace
{

void append_utf8(std::string& output, std::uint32_t scalar)
{
    if (scalar <= 0x7f)
    {
        output.push_back(static_cast<char>(scalar));
    }
    else if (scalar <= 0x7ff)
    {
        output.push_back(static_cast<char>(0xc0 | (scalar >> 6)));
        output.push_back(static_cast<char>(0x80 | (scalar & 0x3f)));
    }
    else if (scalar <= 0xffff)
    {
        output.push_back(static_cast<char>(0xe0 | (scalar >> 12)));
        output.push_back(static_cast<char>(0x80 | ((scalar >> 6) & 0x3f)));
        output.push_back(static_cast<char>(0x80 | (scalar & 0x3f)));
    }
    else
    {
        output.push_back(static_cast<char>(0xf0 | (scalar >> 18)));
        output.push_back(static_cast<char>(0x80 | ((scalar >> 12) & 0x3f)));
        output.push_back(static_cast<char>(0x80 | ((scalar >> 6) & 0x3f)));
        output.push_back(static_cast<char>(0x80 | (scalar & 0x3f)));
    }
}

int hex_digit(char value)
{
    if (value >= '0' && value <= '9')
    {
        return value - '0';
    }
    if (value >= 'a' && value <= 'f')
    {
        return value - 'a' + 10;
    }
    if (value >= 'A' && value <= 'F')
    {
        return value - 'A' + 10;
    }
    return -1;
}

struct reader
{
    std::string_view text;
    std::size_t offset = 0;

    void space()
    {
        while (offset < text.size() &&
               (text[offset] == ' ' || text[offset] == '\t' ||
                text[offset] == '\r' || text[offset] == '\n'))
        {
            ++offset;
        }
    }

    bool take(char expected)
    {
        space();
        if (offset < text.size() && text[offset] == expected)
        {
            ++offset;
            return true;
        }
        return false;
    }

    void require(char expected)
    {
        if (!take(expected))
        {
            throw std::runtime_error("JSON 语法错误");
        }
    }

    std::uint32_t hex_quad()
    {
        std::uint32_t value = 0;
        for (int index = 0; index < 4; ++index)
        {
            if (offset == text.size() || hex_digit(text[offset]) < 0)
            {
                throw std::runtime_error("JSON Unicode 转义无效");
            }
            value = value * 16 + static_cast<std::uint32_t>(hex_digit(text[offset++]));
        }
        return value;
    }

    std::string string()
    {
        require('"');
        std::string result;
        while (offset < text.size())
        {
            const auto value = static_cast<unsigned char>(text[offset++]);
            if (value == '"')
            {
                return result;
            }
            if (value < 0x20)
            {
                throw std::runtime_error("JSON 字符串含控制字符");
            }
            if (value != '\\')
            {
                result.push_back(static_cast<char>(value));
                continue;
            }
            if (offset == text.size())
            {
                throw std::runtime_error("JSON 转义未完成");
            }
            const char escaped = text[offset++];
            switch (escaped)
            {
            case '"': case '\\': case '/': result.push_back(escaped); break;
            case 'b': result.push_back('\b'); break;
            case 'f': result.push_back('\f'); break;
            case 'n': result.push_back('\n'); break;
            case 'r': result.push_back('\r'); break;
            case 't': result.push_back('\t'); break;
            case 'u':
            {
                auto scalar = hex_quad();
                if (scalar >= 0xd800 && scalar <= 0xdbff)
                {
                    if (offset + 2 > text.size() || text.substr(offset, 2) != "\\u")
                    {
                        throw std::runtime_error("JSON 代理对缺少低位");
                    }
                    offset += 2;
                    const auto low = hex_quad();
                    if (low < 0xdc00 || low > 0xdfff)
                    {
                        throw std::runtime_error("JSON 代理对低位无效");
                    }
                    scalar = 0x10000 + ((scalar - 0xd800) << 10) + (low - 0xdc00);
                }
                else if (scalar >= 0xdc00 && scalar <= 0xdfff)
                {
                    throw std::runtime_error("JSON 孤立低位代理");
                }
                append_utf8(result, scalar);
                break;
            }
            default: throw std::runtime_error("JSON 转义无效");
            }
        }
        throw std::runtime_error("JSON 字符串未结束");
    }

    std::int64_t integer()
    {
        space();
        const auto start = offset;
        if (offset < text.size() && text[offset] == '-')
        {
            ++offset;
        }
        if (offset == text.size() || text[offset] < '0' || text[offset] > '9')
        {
            throw std::runtime_error("JSON 整数字段类型无效");
        }
        if (text[offset++] != '0')
        {
            while (offset < text.size() && text[offset] >= '0' && text[offset] <= '9')
            {
                ++offset;
            }
        }
        std::int64_t result = 0;
        const auto parsed = std::from_chars(text.data() + start, text.data() + offset,
                                            result);
        if (parsed.ec != std::errc{} || parsed.ptr != text.data() + offset)
        {
            throw std::runtime_error("JSON 整数超出范围");
        }
        return result;
    }
};

void append_json_string(std::string& output, std::string_view value)
{
    constexpr char hex[] = "0123456789abcdef";
    output.push_back('"');
    for (const auto character : value)
    {
        const auto byte = static_cast<unsigned char>(character);
        if (byte == '"' || byte == '\\')
        {
            output.push_back('\\');
            output.push_back(character);
        }
        else if (byte < 0x20)
        {
            output.append("\\u00");
            output.push_back(hex[byte >> 4]);
            output.push_back(hex[byte & 15]);
        }
        else
        {
            output.push_back(character);
        }
    }
    output.push_back('"');
}

}

std::string serialize_payload(const payload& value)
{
    std::string output = "{\"$schema\":1,\"id\":" + std::to_string(value.id) +
                         ",\"name\":";
    append_json_string(output, value.name);
    output.push_back('}');
    if (output.size() > 16 * 1024 * 1024)
    {
        throw std::runtime_error("serde JSON 输出超过 16 MiB");
    }
    return output;
}

payload deserialize_payload(std::string_view text)
{
    if (text.size() > 16 * 1024 * 1024)
    {
        throw std::runtime_error("serde JSON 输入超过 16 MiB");
    }
    reader input{text};
    input.require('{');
    bool schema_seen = false;
    bool id_seen = false;
    bool name_seen = false;
    payload result;
    do
    {
        const auto key = input.string();
        input.require(':');
        if (key == "$schema" && !schema_seen)
        {
            schema_seen = true;
            if (input.integer() != 1)
            {
                throw std::runtime_error("serde schema 版本不符");
            }
        }
        else if (key == "id" && !id_seen)
        {
            id_seen = true;
            result.id = input.integer();
        }
        else if (key == "name" && !name_seen)
        {
            name_seen = true;
            result.name = input.string();
        }
        else
        {
            throw std::runtime_error("serde 重复或未知字段");
        }
    } while (input.take(','));
    input.require('}');
    input.space();
    if (input.offset != text.size() || !schema_seen || !id_seen || !name_seen)
    {
        throw std::runtime_error("serde 缺少字段或存在多余内容");
    }
    return result;
}

void check_serde_contract()
{
    const payload original{7, "a\"b\\c"};
    const auto decoded = deserialize_payload(serialize_payload(original));
    if (decoded.id != original.id || decoded.name != original.name)
    {
        throw std::runtime_error("serde 往返校验失败");
    }
    constexpr std::string_view invalid[] = {
        "{\"$schema\":2,\"id\":7,\"name\":\"x\"}",
        "{\"$schema\":1,\"id\":7,\"id\":8,\"name\":\"x\"}",
        "{\"$schema\":1,\"id\":7,\"name\":\"x\",\"extra\":1}",
        "{\"$schema\":1,\"id\":\"7\",\"name\":\"x\"}",
        "{\"$schema\":1,\"id\":7}",
    };
    for (const auto text : invalid)
    {
        try
        {
            (void)deserialize_payload(text);
        }
        catch (const std::runtime_error&)
        {
            continue;
        }
        throw std::runtime_error("serde 应拒绝无效 schema 输入");
    }
}

}
