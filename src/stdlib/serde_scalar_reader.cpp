#include "stdlib/serde_direct.hpp"

namespace tx_generated
{
namespace
{
template<class type>
type require_scalar(std::any value)
{
    if (auto* result = std::any_cast<type>(&value))
    {
        return std::move(*result);
    }
    serde_decode_error("type_mismatch", "serde 字段值与声明类型不匹配");
}
}

std::int64_t serde_reader::integer(std::size_t depth)
{
    if (format != serde_format::json)
    {
        return require_scalar<std::int64_t>(scalar(depth));
    }
    json_.skip_space();
    const auto number = json_.parse_numeric();
    if (const auto* result = std::get_if<std::int64_t>(&number))
    {
        return *result;
    }
    serde_decode_error("type_mismatch", "serde 字段值与声明类型不匹配");
}

double serde_reader::floating(std::size_t depth)
{
    if (format != serde_format::json)
    {
        return require_scalar<double>(scalar(depth));
    }
    json_.skip_space();
    const auto number = json_.parse_numeric();
    if (const auto* result = std::get_if<double>(&number))
    {
        return *result;
    }
    serde_decode_error("type_mismatch", "serde 字段值与声明类型不匹配");
}

bool serde_reader::boolean(std::size_t depth)
{
    if (format != serde_format::json)
    {
        return require_scalar<bool>(scalar(depth));
    }
    json_.skip_space();
    const bool result = input_.peek() == 't';
    const std::string_view word = result ? "true" : "false";
    for (const auto byte : word)
    {
        if (!input_.take(byte))
        {
            serde_decode_error("type_mismatch", "serde 字段值与声明类型不匹配");
        }
    }
    return result;
}

std::string serde_reader::text(std::size_t depth)
{
    if (format != serde_format::json)
    {
        return require_scalar<std::string>(scalar(depth));
    }
    json_.skip_space();
    if (input_.peek() != '"')
    {
        serde_decode_error("type_mismatch", "serde 字段值与声明类型不匹配");
    }
    return json_.parse_string();
}

} // namespace tx_generated
