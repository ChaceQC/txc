#include "stdlib/json_stream.hpp"
#include "stdlib/json_utf8.hpp"

#include <cmath>
#include <unordered_set>

namespace tx_generated
{
namespace
{

[[noreturn]] void schema_error(const char* code, const std::string& path,
                              const char* reason)
{
    throw runtime_failure({tx::error_kind::parse, code, "JSON schema " + path + "：" + reason});
}

const std::any* field(const tx_dict& schema, std::string_view key)
{
    return schema.find_value(key);
}

bool numeric(const std::any& value)
{
    return value.type() == typeid(std::int64_t) || value.type() == typeid(double);
}

long double number(const std::any& value)
{
    if (const auto* integer = std::any_cast<std::int64_t>(&value))
    {
        return *integer;
    }
    return std::any_cast<double>(value);
}

bool matches_type(const std::any& value, std::string_view name)
{
    return (name == "null" && !value.has_value()) ||
        (name == "boolean" && value.type() == typeid(bool)) ||
        (name == "integer" && value.type() == typeid(std::int64_t)) ||
        (name == "number" && numeric(value)) ||
        (name == "string" && value.type() == typeid(std::string)) ||
        (name == "array" && value.type() == typeid(tx_array)) ||
        (name == "object" && value.type() == typeid(tx_dict));
}

std::string child_path(const std::string& path, std::string_view key)
{
    std::string result = path + "/";
    for (const char byte : key)
    {
        result += byte == '~' ? "~0" : byte == '/' ? "~1" : std::string(1, byte);
    }
    return result;
}

void check_schema(const tx_dict& schema, std::size_t depth, std::size_t& nodes);

void check_keyword(std::string_view key, const std::any& value,
                   std::size_t depth, std::size_t& nodes)
{
    bool valid = false;
    if (key == "type")
    {
        const auto* name = std::any_cast<std::string>(&value);
        valid = name && (*name == "null" || *name == "boolean" || *name == "integer" ||
            *name == "number" || *name == "string" || *name == "array" || *name == "object");
    }
    else if (key == "properties")
    {
        if (const auto* properties = std::any_cast<tx_dict>(&value))
        {
            valid = true;
            properties->for_each([&](const std::any&, const std::any& item)
            {
                const auto* child = std::any_cast<tx_dict>(&item);
                if (!child)
                {
                    schema_error("invalid_schema", "$", "properties 的每个值必须是 schema 对象");
                }
                check_schema(*child, depth + 1, nodes);
            });
        }
    }
    else if (key == "items")
    {
        if (const auto* child = std::any_cast<tx_dict>(&value))
        {
            valid = true;
            check_schema(*child, depth + 1, nodes);
        }
    }
    else if (key == "required")
    {
        if (const auto* names = std::any_cast<tx_array>(&value))
        {
            valid = true;
            std::unordered_set<std::string> seen;
            for (std::size_t index = 0; index < names->size(); ++index)
            {
                const auto* name = std::any_cast<std::string>(&(*names)[index]);
                if (!name || !seen.insert(*name).second)
                {
                    valid = false;
                    break;
                }
            }
        }
    }
    else if (key == "additionalProperties")
    {
        valid = value.type() == typeid(bool);
    }
    else if (key == "minimum" || key == "maximum")
    {
        valid = numeric(value) && std::isfinite(number(value));
    }
    else if (key == "minLength" || key == "maxLength" || key == "minItems" || key == "maxItems")
    {
        const auto* count = std::any_cast<std::int64_t>(&value);
        valid = count && *count >= 0;
    }
    if (!valid)
    {
        schema_error("invalid_schema", "$", "存在不支持的关键字或无效约束类型");
    }
}

void check_schema(const tx_dict& schema, std::size_t depth, std::size_t& nodes)
{
    if (depth > 128 || ++nodes > 10000)
    {
        schema_error("invalid_schema", "$", "schema 超过深度或节点上限");
    }
    schema.for_each([&](const std::any& key, const std::any& value)
    {
        check_keyword(std::any_cast<const std::string&>(key), value, depth, nodes);
    });
    for (const auto& pair : {std::pair{"minimum", "maximum"},
                            std::pair{"minLength", "maxLength"},
                            std::pair{"minItems", "maxItems"}})
    {
        const auto* low = field(schema, pair.first);
        const auto* high = field(schema, pair.second);
        if (low && high && number(*low) > number(*high))
        {
            schema_error("invalid_schema", "$", "约束下界不能超过上界");
        }
    }
}

void check_range(long double value, const tx_dict& schema, const char* low,
                  const char* high, const std::string& path)
{
    const auto* minimum = field(schema, low);
    const auto* maximum = field(schema, high);
    if ((minimum && value < number(*minimum)) || (maximum && value > number(*maximum)))
    {
        schema_error("schema_mismatch", path, "数值或长度超出 schema 范围");
    }
}

void validate_value(const std::any& value, const tx_dict& schema,
                    const std::string& path, std::size_t& nodes);

void validate_object(const tx_dict& value, const tx_dict& schema,
                     const std::string& path, std::size_t& nodes)
{
    if (const auto* required = field(schema, "required"))
    {
        const auto& names = std::any_cast<const tx_array&>(*required);
        for (std::size_t index = 0; index < names.size(); ++index)
        {
            const auto& name = std::any_cast<const std::string&>(names[index]);
            if (!value.find_value(std::string_view(name)))
            {
                schema_error("missing_field", child_path(path, name), "缺少必需字段");
            }
        }
    }
    const auto* property_value = field(schema, "properties");
    const auto* properties = property_value ? std::any_cast<tx_dict>(property_value) : nullptr;
    const auto* additional = field(schema, "additionalProperties");
    value.for_each([&](const std::any& key, const std::any& item)
    {
        const auto& name = std::any_cast<const std::string&>(key);
        const auto* child = properties ? properties->find_value(std::string_view(name)) : nullptr;
        if (child)
        {
            validate_value(item, std::any_cast<const tx_dict&>(*child), child_path(path, name), nodes);
        }
        else if (additional && !std::any_cast<bool>(*additional))
        {
            schema_error("schema_mismatch", child_path(path, name), "不允许未声明字段");
        }
    });
}

void validate_value(const std::any& value, const tx_dict& schema,
                    const std::string& path, std::size_t& nodes)
{
    if (++nodes > 1000000)
    {
        schema_error("size_limit", path, "校验超过节点上限");
    }
    if (const auto* expected = field(schema, "type"))
    {
        if (!matches_type(value, std::any_cast<const std::string&>(*expected)))
        {
            schema_error("type_mismatch", path, "值类型不符合 schema");
        }
    }
    if (numeric(value))
    {
        check_range(number(value), schema, "minimum", "maximum", path);
    }
    if (const auto* text = std::any_cast<std::string>(&value))
    {
        std::size_t count = 0;
        for (std::size_t offset = 0; offset < text->size(); ++count)
        {
            offset += json_detail::utf8_width(*text, offset);
        }
        check_range(count, schema, "minLength", "maxLength", path);
    }
    if (const auto* array = std::any_cast<tx_array>(&value))
    {
        check_range(array->size(), schema, "minItems", "maxItems", path);
        if (const auto* items = field(schema, "items"))
        {
            for (std::size_t index = 0; index < array->size(); ++index)
            {
                validate_value((*array)[index], std::any_cast<const tx_dict&>(*items),
                    child_path(path, std::to_string(index)), nodes);
            }
        }
    }
    if (const auto* object = std::any_cast<tx_dict>(&value))
    {
        validate_object(*object, schema, path, nodes);
    }
}

} // namespace

void json_validate(const std::any& value, const tx_dict& schema)
{
    const auto discard = [](std::string_view)
    {
    };
    try
    {
        // 同一生成器复用类型、UTF-8、深度和引用环检查，不产生中间 JSON 文本。
        json_emit(schema, discard, 67108864, 128, 0, 10000);
    }
    catch (const runtime_failure&)
    {
        schema_error("invalid_schema", "$", "schema 不是受限的无环 JSON 对象");
    }
    std::size_t nodes = 0;
    check_schema(schema, 0, nodes);
    try
    {
        json_emit(value, discard, 67108864, 128, 0, 1000000);
    }
    catch (const runtime_failure& error)
    {
        schema_error(error.error().code.c_str(), "$", "校验值不是受限的无环 JSON 值");
    }
    nodes = 0;
    validate_value(value, schema, "$", nodes);
}

} // namespace tx_generated
