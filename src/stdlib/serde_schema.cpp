#include "stdlib/serde.hpp"

#include "stdlib/array.hpp"
#include "stdlib/dictionary.hpp"
#include "stdlib/json.hpp"

#include <stdexcept>

namespace tx_generated
{
namespace
{

const std::any& member(const tx_dict& source, std::string_view name)
{
    const auto* value = source.find_value(name);
    if (value == nullptr)
    {
        throw std::runtime_error("编译器生成的 serde schema 缺少属性");
    }
    return *value;
}

std::int64_t number(const tx_dict& source, std::string_view name)
{
    return std::any_cast<std::int64_t>(member(source, name));
}

std::string text(const tx_dict& source, std::string_view name)
{
    return std::any_cast<std::string>(member(source, name));
}

std::shared_ptr<serde_schema> parse_struct(const tx_dict& source);

serde_type parse_type(const tx_dict& source)
{
    const auto kind = text(source, "kind");
    serde_type result;
    if (kind == "int")
    {
        result.kind = serde_kind::integer;
    }
    else if (kind == "float")
    {
        result.kind = serde_kind::floating;
    }
    else if (kind == "bool")
    {
        result.kind = serde_kind::boolean;
    }
    else if (kind == "str")
    {
        result.kind = serde_kind::text;
    }
    else if (kind == "bytes")
    {
        result.kind = serde_kind::bytes;
    }
    else if (kind == "vector" || kind == "option")
    {
        result.kind = kind == "vector" ? serde_kind::vector : serde_kind::option;
        result.name = text(source, "name");
        result.element = std::make_shared<serde_type>(parse_type(
            std::any_cast<const tx_dict&>(member(source, "element"))));
    }
    else if (kind == "struct")
    {
        result.kind = serde_kind::structure;
        result.structure = parse_struct(
            std::any_cast<const tx_dict&>(member(source, "schema")));
    }
    else
    {
        throw std::runtime_error("编译器生成了未知的 serde 字段类型");
    }
    return result;
}

std::shared_ptr<serde_schema> parse_struct(const tx_dict& source)
{
    auto schema = std::make_shared<serde_schema>();
    schema->type_name = text(source, "type");
    schema->display_name = text(source, "display");
    schema->version = number(source, "version");
    const auto policy = text(source, "unknown");
    schema->unknown = policy == "preserve" ? serde_unknown::preserve :
        policy == "ignore" ? serde_unknown::ignore : serde_unknown::reject;
    schema->field_count = static_cast<std::size_t>(number(source, "field_count"));
    const auto unknown_index = number(source, "unknown_index");
    if (unknown_index >= 0)
    {
        schema->unknown_index = static_cast<std::size_t>(unknown_index);
    }
    schema->unknown_name = text(source, "unknown_name");
    const auto& fields = std::any_cast<const tx_array&>(member(source, "fields"));
    schema->fields.reserve(fields.size());
    for (const auto& item : fields)
    {
        const auto& object = std::any_cast<const tx_dict&>(item);
        serde_field field;
        field.name = text(object, "name");
        field.number = number(object, "number");
        field.index = static_cast<std::size_t>(number(object, "index"));
        field.type = parse_type(std::any_cast<const tx_dict&>(
            member(object, "type")));
        if (const auto* default_value = object.find_value(
                std::string_view("default")))
        {
            field.default_value = *default_value;
        }
        schema->fields.push_back(std::move(field));
    }
    return schema;
}

} // namespace

std::shared_ptr<serde_schema> serde_parse_schema(std::string_view source)
{
    return parse_struct(std::any_cast<const tx_dict&>(json_parse(source)));
}

} // namespace tx_generated
