#include "backend/llvm/codegen.hpp"

#include <algorithm>
#include <functional>
#include <string>
#include <string_view>

namespace tx
{
namespace
{

std::string json_string(std::string_view input)
{
    std::string result = "\"";
    constexpr char digits[] = "0123456789abcdef";
    for (const unsigned char byte : input)
    {
        if (byte == '"' || byte == '\\')
        {
            result += '\\';
            result += static_cast<char>(byte);
        }
        else if (byte < 0x20)
        {
            result += "\\u00";
            result += digits[byte >> 4];
            result += digits[byte & 15];
        }
        else
        {
            result += static_cast<char>(byte);
        }
    }
    return result + '"';
}

} // namespace

std::string llvm_code_generator::serde_schema_json(const value_type& type) const
{
    std::function<std::string(const value_type&)> type_json;
    std::function<std::string(const value_type&)> struct_json;
    type_json = [&](const value_type& value) -> std::string
    {
        if (value.is_vector() || value.is_option())
        {
            return "{\"kind\":" + json_string(value.container_name()) +
                ",\"name\":" + json_string(value.name) +
                ",\"element\":" + type_json(value.parameters.front()) + "}";
        }
        if (structs_.contains(value.name))
        {
            return "{\"kind\":\"struct\",\"schema\":" +
                struct_json(value) + "}";
        }
        return "{\"kind\":" + json_string(value.name) + "}";
    };
    struct_json = [&](const value_type& value) -> std::string
    {
        const auto& definition = *structs_.at(value.name);
        const auto& metadata = *definition.serde;
        const auto policy = metadata.unknown == serde_unknown_policy::preserve
            ? "preserve" : metadata.unknown == serde_unknown_policy::ignore
            ? "ignore" : "reject";
        std::string unknown_name;
        std::string unknown_index = "-1";
        for (std::size_t index = 0; index < definition.fields.size(); ++index)
        {
            const auto& field = definition.fields[index];
            if (field.serde->unknown_capture)
            {
                unknown_name = field.name;
                unknown_index = std::to_string(index);
            }
        }
        std::string result = "{\"type\":" + json_string(definition.name) +
            ",\"display\":" + json_string(definition.source_name.empty()
                ? definition.name : definition.source_name) +
            ",\"version\":" + std::to_string(metadata.version) +
            ",\"unknown\":" + json_string(policy) +
            ",\"field_count\":" + std::to_string(definition.fields.size()) +
            ",\"unknown_index\":" + unknown_index +
            ",\"unknown_name\":" +
            json_string(unknown_name) + ",\"fields\":[";
        bool first = true;
        for (std::size_t index = 0; index < definition.fields.size(); ++index)
        {
            const auto& field = definition.fields[index];
            if (field.serde->unknown_capture)
            {
                continue;
            }
            if (!first)
            {
                result += ',';
            }
            first = false;
            result += "{\"name\":" + json_string(field.name) +
                ",\"number\":" + std::to_string(field.serde->number) +
                ",\"index\":" + std::to_string(index) +
                ",\"type\":" + type_json(field.type);
            if (field.serde->default_literal)
            {
                result += ",\"default\":" + *field.serde->default_literal;
            }
            result += '}';
        }
        return result + "]}";
    };
    return struct_json(type);
}

llvm_code_generator::ir_value llvm_code_generator::emit_serde_intrinsic(
    const expression& item, const function_decl& target,
    const std::vector<ir_value>& arguments)
{
    const bool decoding = target.external_name.starts_with("serde.deserialize_");
    const auto& schema_type = decoding ? item.type : arguments.front().type;
    const auto schema = global_bytes(serde_schema_json(schema_type));
    std::string symbol = "@txrt_" + target.external_name;
    std::replace(symbol.begin(), symbol.end(), '.', '_');
    const auto output = allocate(item.type, item.position);
    const auto status = temporary();
    write_instruction(status + " = call i32 " + symbol + "(ptr " + schema +
        ", ptr " + arguments.front().text + ", ptr " + output + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    release(arguments.front());
    const auto result = temporary();
    write_instruction(result + " = load ptr, ptr " + output);
    return {item.type, result};
}

} // namespace tx
