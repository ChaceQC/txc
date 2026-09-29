#include "stdlib/format_internal.hpp"
#include "common/utf8.hpp"

#include <charconv>
#include <stdexcept>

namespace tx_generated
{

void append_format_integer(std::string& output, std::int64_t value)
{
    char buffer[20];
    const auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer), value);
    if (error != std::errc{})
    {
        throw std::runtime_error("format 整数格式化失败");
    }
    output.append(buffer, end);
}

void append_format_bool(std::string& output, bool value)
{
    output.append(value ? "true" : "false");
}

void append_format_text(std::string& output, std::string_view value)
{
    // 无宽度时不计算码点个数；仍验证编码，保持原 apply_width 的错误语义。
    for (std::size_t offset = 0; offset < value.size();)
    {
        const auto width = tx::utf8_width(value, offset);
        if (width == 0)
        {
            throw std::runtime_error("字符串包含无效 UTF-8");
        }
        offset += width;
    }
    output.append(value);
}

void append_format_value(std::string& output, std::int64_t value,
    const tx::format_spec& spec, char conversion)
{
    if (tx::format_plain_integer(spec, conversion))
    {
        append_format_integer(output, value);
        return;
    }
    output += format_field_value(value, spec, conversion);
}

void append_format_value(std::string& output, bool value,
    const tx::format_spec& spec, char conversion)
{
    if (tx::format_plain_bool(spec, conversion))
    {
        append_format_bool(output, value);
        return;
    }
    output += format_field_value(value, spec, conversion);
}

void append_format_value(std::string& output, const std::string& value,
    const tx::format_spec& spec, char conversion)
{
    if (tx::format_plain_text(spec, conversion))
    {
        append_format_text(output, value);
        return;
    }
    output += format_field_value(value, spec, conversion);
}

void append_format_value(std::string& output, double value,
    const tx::format_spec& spec, char conversion)
{
    output += format_field_value(value, spec, conversion);
}

void append_format_value(std::string& output, const std::any& value,
    const tx::format_spec& spec, char conversion)
{
    if (const auto* integer = std::any_cast<std::int64_t>(&value))
    {
        append_format_value(output, *integer, spec, conversion);
    }
    else if (const auto* boolean = std::any_cast<bool>(&value))
    {
        append_format_value(output, *boolean, spec, conversion);
    }
    else if (const auto* text = std::any_cast<std::string>(&value))
    {
        append_format_value(output, *text, spec, conversion);
    }
    else
    {
        output += format_field_value(value, spec, conversion);
    }
}

} // namespace tx_generated
