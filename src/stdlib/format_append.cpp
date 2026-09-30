#include "stdlib/format_internal.hpp"
#include "common/utf8.hpp"

#include <charconv>
#include <algorithm>
#include <stdexcept>

namespace tx_generated
{
namespace
{

void append_integer_field(std::string& output, std::int64_t value,
    const tx::format_spec& spec)
{
    const auto align = spec.align == 0 ? '>' : spec.align;
    if (spec.zero && align != '>')
    {
        throw std::runtime_error("format 数字零填充只支持右对齐");
    }
    const int base = spec.type == 'b' ? 2 : spec.type == 'o' ? 8 :
        spec.type == 'x' || spec.type == 'X' ? 16 : 10;
    char buffer[71];
    auto* start = buffer;
    if (value >= 0 && spec.sign)
    {
        *start++ = static_cast<char>(spec.sign);
    }
    const auto [end, error] = std::to_chars(start, buffer + sizeof(buffer), value, base);
    if (error != std::errc{})
    {
        throw std::runtime_error("format 整数格式化失败");
    }
    if (spec.type == 'X')
    {
        std::transform(start, end, start, [](char item)
        {
            return item >= 'a' && item <= 'f' ? static_cast<char>(item - 'a' + 'A') : item;
        });
    }
    const auto length = static_cast<std::size_t>(end - buffer);
    const auto padding = spec.width > length ? spec.width - length : 0;
    const auto left = align == '<' ? 0 : align == '^' ? padding / 2 : padding;
    const auto fill = static_cast<char>(spec.zero ? '0' : spec.fill);
    // 数字表示必为 ASCII，无需再次扫描 Unicode；零填充仍位于符号之后。
    start = buffer;
    if (spec.zero && (*start == '-' || *start == '+' || *start == ' '))
    {
        output.push_back(*start++);
    }
    output.append(left, fill);
    output.append(start, end);
    output.append(padding - left, fill);
}

} // namespace

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
    if (!tx::scan_utf8(value).valid)
    {
        throw std::runtime_error("字符串包含无效 UTF-8");
    }
    output.append(value);
}

void append_format_value(std::string& output, std::int64_t value,
    const tx::format_spec& spec, char conversion)
{
    if (conversion == 0 && spec.precision < 0 &&
        (spec.type == 0 || spec.type == 'd' || spec.type == 'b' ||
         spec.type == 'o' || spec.type == 'x' || spec.type == 'X'))
    {
        append_integer_field(output, value, spec);
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
