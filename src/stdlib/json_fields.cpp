#include "stdlib/json.hpp"

#include <any>
#include <cstdint>
#include <string>
#include <string_view>

namespace tx_generated
{
namespace
{

[[noreturn]] void field_error(const char* code, std::string_view key,
                              std::string_view reason)
{
    throw runtime_failure({tx::error_kind::parse, code,
        "JSON 字段 \"" + std::string(key) + "\"" + std::string(reason)});
}

const std::any& required_field(const tx_dict& object, std::string_view key)
{
    const auto* value = object.find_value(key);
    if (!value)
    {
        field_error("missing_field", key, "不存在");
    }
    return *value;
}

template<class value_type>
value_type typed_field(const tx_dict& object, std::string_view key,
                       std::string_view expected)
{
    const auto& value = required_field(object, key);
    if (const auto* matched = std::any_cast<value_type>(&value))
    {
        return *matched;
    }
    field_error("type_mismatch", key, "需要 " + std::string(expected));
}

} // namespace

tx_dict json_parse_object(std::string_view text)
{
    auto value = json_parse(text);
    if (const auto* object = std::any_cast<tx_dict>(&value))
    {
        return *object;
    }
    throw runtime_failure({tx::error_kind::parse, "type_mismatch",
                           "JSON 根值需要 dict 对象"});
}

bool json_contains(const tx_dict& object, std::string_view key)
{
    return object.find_value(key) != nullptr;
}

std::any json_get(const tx_dict& object, std::string_view key)
{
    return required_field(object, key);
}

std::int64_t json_get_int(const tx_dict& object, std::string_view key)
{
    return typed_field<std::int64_t>(object, key, "int");
}

double json_get_float(const tx_dict& object, std::string_view key)
{
    const auto& value = required_field(object, key);
    if (const auto* floating = std::any_cast<double>(&value))
    {
        return *floating;
    }
    if (const auto* integer = std::any_cast<std::int64_t>(&value))
    {
        return static_cast<double>(*integer);
    }
    field_error("type_mismatch", key, "需要数字");
}

bool json_get_bool(const tx_dict& object, std::string_view key)
{
    return typed_field<bool>(object, key, "bool");
}

std::string json_get_str(const tx_dict& object, std::string_view key)
{
    return typed_field<std::string>(object, key, "str");
}

tx_array json_get_array(const tx_dict& object, std::string_view key)
{
    return typed_field<tx_array>(object, key, "array");
}

tx_dict json_get_object(const tx_dict& object, std::string_view key)
{
    return typed_field<tx_dict>(object, key, "dict");
}

} // namespace tx_generated
