#include "backend/llvm/codegen.hpp"

#include <algorithm>
#include <bit>
#include <charconv>
#include <numeric>

namespace tx
{

std::string llvm_code_generator::serde_type_constant(const value_type& type)
{
    const auto kind = type == value_type::int_type ? 0 :
        type == value_type::float_type ? 1 : type == value_type::bool_type ? 2 :
        type == value_type::str_type ? 3 : type == value_type::bytes_type ? 4 :
        type.is_vector() ? 5 : type.is_option() ? 6 : 7;
    std::string element = "null";
    std::string structure = "null";
    if (type.is_vector() || type.is_option())
    {
        const auto& child = type.parameters.front();
        const auto found = serde_types_.find(child.name);
        if (found != serde_types_.end())
        {
            element = found->second;
        }
        else
        {
            element = "@.serde_type." + std::to_string(serde_types_.size());
            serde_types_.emplace(child.name, element);
            const auto value = serde_type_constant(child);
            globals_ << element << " = private constant { i64, ptr, ptr, ptr, ptr } "
                     << value << '\n';
        }
    }
    else if (kind == 7)
    {
        structure = serde_schema_constant(type);
    }
    constexpr const char* codecs[] = {"integer", "floating", "boolean", "text",
        "bytes", "vector_object", "option", "structure"};
    std::string codec = codecs[kind];
    if (type.is_vector())
    {
        const auto& child = type.parameters.front();
        codec = "vector_" + std::string(child == value_type::int_type ? "integer" :
            child == value_type::float_type ? "floating" :
            child == value_type::bool_type ? "boolean" :
            child == value_type::str_type ? "text" :
            child == value_type::bytes_type ? "bytes" : "object");
    }
    return "{ i64 " + std::to_string(kind) + ", ptr " + global_bytes(type.name) +
        ", ptr " + element + ", ptr " + structure + ", ptr @tx_serde_" + codec + " }";
}

std::string llvm_code_generator::serde_default_constant(const struct_field& field)
{
    if (!field.serde->default_literal)
    {
        return "{ i64 -1, i64 0, ptr null, i64 0 }";
    }
    const auto& literal = *field.serde->default_literal;
    std::int64_t bits = 0;
    std::int64_t kind = 0;
    std::string text = "null";
    std::size_t length = 0;
    if (field.type == value_type::str_type)
    {
        kind = 3;
        const auto decoded = decode_string_literal(literal);
        text = global_bytes(decoded);
        length = decoded.size();
    }
    else if (field.type == value_type::bool_type)
    {
        kind = 2;
        bits = literal == "true";
    }
    else if (field.type == value_type::float_type)
    {
        kind = 1;
        double value = 0;
        std::from_chars(literal.data(), literal.data() + literal.size(), value);
        bits = std::bit_cast<std::int64_t>(value);
    }
    else
    {
        std::from_chars(literal.data(), literal.data() + literal.size(), bits);
    }
    return "{ i64 " + std::to_string(kind) + ", i64 " + std::to_string(bits) +
        ", ptr " + text + ", i64 " + std::to_string(length) + " }";
}

std::string llvm_code_generator::serde_schema_constant(const value_type& type)
{
    if (const auto found = serde_schemas_.find(type.name); found != serde_schemas_.end())
    {
        return found->second;
    }
    const auto symbol = "@.serde_schema." + std::to_string(serde_schemas_.size());
    serde_schemas_.emplace(type.name, symbol);
    const auto& definition = *structs_.at(type.name);
    const auto& metadata = *definition.serde;
    constexpr std::string_view field_layout =
        "{ ptr, i64, i64, { i64, ptr, ptr, ptr, ptr }, { i64, i64, ptr, i64 } }";
    std::string fields;
    std::size_t count = 0;
    std::int64_t unknown_index = -1;
    std::string unknown_name;
    std::vector<const struct_field*> ordered_fields;
    for (std::size_t index = 0; index < definition.fields.size(); ++index)
    {
        const auto& field = definition.fields[index];
        if (field.serde->unknown_capture)
        {
            unknown_index = static_cast<std::int64_t>(index);
            unknown_name = field.name;
            continue;
        }
        if (count++ != 0)
        {
            fields += ", ";
        }
        ordered_fields.push_back(&field);
        fields += std::string(field_layout) + " { ptr " + global_bytes(field.name) +
            ", i64 " + std::to_string(field.serde->number) + ", i64 " +
            std::to_string(index) + ", { i64, ptr, ptr, ptr, ptr } " +
            serde_type_constant(field.type) + ", { i64, i64, ptr, i64 } " +
            serde_default_constant(field) + " }";
    }
    const auto type_name = global_bytes(type.name);
    const auto display = global_bytes(definition.source_name.empty() ? type.name : definition.source_name);
    const auto unknown = global_bytes(unknown_name);
    const auto policy = metadata.unknown == serde_unknown_policy::preserve ? 2 :
        metadata.unknown == serde_unknown_policy::ignore ? 1 : 0;
    // JSON 按 UTF-8 字段名字节序，CBOR 正整数键按规范编码序（即编号序）。
    std::vector<std::size_t> order(count);
    std::iota(order.begin(), order.end(), 0);
    for (const bool json : {true, false})
    {
        std::sort(order.begin(), order.end(), [&](std::size_t left, std::size_t right)
        {
            return json ? ordered_fields[left]->name < ordered_fields[right]->name
                : ordered_fields[left]->serde->number < ordered_fields[right]->serde->number;
        });
        globals_ << symbol << (json ? ".json_order" : ".cbor_order")
                 << " = private constant [" << count << " x i64] [";
        for (std::size_t index = 0; index < count; ++index)
        {
            globals_ << (index ? ", " : "") << "i64 " << order[index];
        }
        globals_ << "]\n";
    }
    globals_ << symbol << ".fields = private constant [" << count << " x "
             << field_layout << "] [" << fields << "]\n"
             << symbol << " = private constant { ptr, ptr, i64, i64, i64, i64, ptr, ptr, i64, ptr, ptr } "
             << "{ ptr " << type_name << ", ptr " << display << ", i64 " << metadata.version
             << ", i64 " << policy << ", i64 " << definition.fields.size()
             << ", i64 " << unknown_index << ", ptr " << unknown << ", ptr "
             << symbol << ".fields, i64 " << count << ", ptr " << symbol
             << ".json_order, ptr " << symbol << ".cbor_order }\n";
    return symbol;
}

llvm_code_generator::ir_value llvm_code_generator::emit_serde_intrinsic(
    const expression& item, const function_decl& target,
    const std::vector<ir_value>& arguments)
{
    const bool decoding = target.external_name.starts_with("serde.deserialize_");
    const auto& schema_type = decoding ? item.type : arguments.front().type;
    const auto schema = serde_schema_constant(schema_type);
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
