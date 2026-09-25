#include "backend/llvm/codegen.hpp"

#include <bit>
#include <cstdint>
#include <iomanip>
#include <sstream>

namespace tx
{
namespace
{

std::string decode_string_literal(std::string_view quoted)
{
    std::string decoded;
    for (std::size_t index = 1; index + 1 < quoted.size(); ++index)
    {
        if (quoted[index] != '\\')
        {
            decoded += quoted[index];
            continue;
        }
        ++index;
        switch (quoted[index])
        {
        case 'n': decoded += '\n'; break;
        case 'r': decoded += '\r'; break;
        case 't': decoded += '\t'; break;
        default: decoded += quoted[index]; break;
        }
    }
    return decoded;
}

} // namespace

llvm_code_generator::ir_value llvm_code_generator::emit_cast(
    const expression& item, const cast_expression& cast)
{
    const auto value = expression_value(*cast.value);
    if (classes_.contains(cast.target.name))
    {
        if (!class_is_assignable(value.type, cast.target))
        {
            const auto status = temporary();
            write_instruction(status + " = call i32 @txrt_class_require_type(ptr " +
                              value.text + ", ptr " +
                              global_bytes(cast.target.name) + ")");
            write_instruction("call void @txrt_require_success(i32 " + status + ")");
        }
        return {cast.target, value.text};
    }
    if (value.type == cast.target)
    {
        return value;
    }
    if (value.type == value_type::any_type)
    {
        const auto result = from_any(value, cast.target, item.position);
        release(value);
        return result;
    }
    if (value.type == value_type::int_type &&
        cast.target == value_type::float_type)
    {
        const auto result = temporary();
        write_instruction(result + " = sitofp i64 " + value.text + " to double");
        return {item.type, result};
    }
    if (value.type == value_type::float_type &&
        cast.target == value_type::int_type)
    {
        const auto address = allocate(value_type::int_type, item.position);
        const auto status = temporary();
        write_instruction(status + " = call i32 @txrt_float_to_int(double " +
                          value.text + ", ptr " + address + ")");
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
        return load({value_type::int_type, address});
    }
    const auto* conversion = value.type == value_type::str_type &&
                              cast.target == value_type::int_type
        ? "txrt_parse_int" : value.type == value_type::str_type &&
                             cast.target == value_type::float_type
        ? "txrt_parse_float" : value.type == value_type::int_type &&
                               cast.target == value_type::str_type
        ? "txrt_int_to_str" : value.type == value_type::float_type &&
                               cast.target == value_type::str_type
        ? "txrt_float_to_str" : value.type == value_type::bool_type &&
                                cast.target == value_type::str_type
        ? "txrt_bool_to_str" : nullptr;
    if (conversion)
    {
        const auto address = allocate(cast.target, item.position);
        const auto status = temporary();
        write_instruction(status + " = call i32 @" + conversion + "(" +
                          llvm_type(value.type, item.position) + " " +
                          value.text + ", ptr " + address + ")");
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
        release(value);
        if (cast.target == value_type::str_type)
        {
            const auto result = temporary();
            write_instruction(result + " = load ptr, ptr " + address);
            return {cast.target, result};
        }
        return load({cast.target, address});
    }
    throw compile_error(item.position, "LLVM 后端暂不支持此类型转换");
}

llvm_code_generator::ir_value llvm_code_generator::expression_value(
    const expression& item)
{
    if (const auto* literal = std::get_if<integer_literal>(&item.data))
    {
        return {item.type, std::to_string(std::stoll(literal->digits))};
    }
    if (const auto* literal = std::get_if<boolean_literal>(&item.data))
    {
        return {item.type, literal->value ? "true" : "false"};
    }
    if (const auto* literal = std::get_if<floating_literal>(&item.data))
    {
        const auto bits = std::bit_cast<std::uint64_t>(std::stod(literal->digits));
        std::ostringstream number;
        number << "0x" << std::hex << std::setw(16) << std::setfill('0') << bits;
        return {item.type, number.str()};
    }
    if (const auto* literal = std::get_if<string_literal>(&item.data))
    {
        const auto decoded = decode_string_literal(literal->text);
        const auto name = global_bytes(decoded);
        const auto address = allocate(value_type::str_type, item.position);
        const auto status = temporary();
        write_instruction(status + " = call i32 @txrt_str_new(ptr " + name +
                          ", i64 " + std::to_string(decoded.size()) +
                          ", ptr " + address + ")");
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
        const auto result = temporary();
        write_instruction(result + " = load ptr, ptr " + address);
        return {item.type, result};
    }
    if (std::holds_alternative<none_literal>(item.data))
    {
        const auto address = allocate(value_type::none_type, item.position);
        const auto status = temporary();
        write_instruction(status + " = call i32 @txrt_value_none(ptr " +
                          address + ")");
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
        const auto result = temporary();
        write_instruction(result + " = load ptr, ptr " + address);
        return {item.type, result};
    }
    if (const auto* literal = std::get_if<array_literal>(&item.data))
    {
        const auto address = allocate(value_type::array_type, item.position);
        const auto status = temporary();
        write_instruction(status + " = call i32 @txrt_array_new(i64 " +
                          std::to_string(literal->elements.size()) +
                          ", ptr " + address + ")");
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
        const auto result = temporary();
        write_instruction(result + " = load ptr, ptr " + address);
        for (std::size_t index = 0; index < literal->elements.size(); ++index)
        {
            const auto element = expression_value(*literal->elements[index]);
            const auto boxed = box_any(element, item.position);
            const auto field = allocate(value_type::any_type, item.position);
            const auto index_status = temporary();
            write_instruction(index_status +
                " = call i32 @txrt_array_element_address(ptr " + result +
                ", i64 " + std::to_string(index) + ", ptr " + field + ")");
            write_instruction("call void @txrt_require_success(i32 " +
                              index_status + ")");
            const auto target = temporary();
            write_instruction(target + " = load ptr, ptr " + field);
            const auto set_status = temporary();
            write_instruction(set_status + " = call i32 @txrt_value_assign(ptr " +
                              target + ", ptr " + boxed.text + ")");
            write_instruction("call void @txrt_require_success(i32 " +
                              set_status + ")");
            release(boxed);
            release(element);
        }
        return {item.type, result};
    }
    if (const auto* literal = std::get_if<dictionary_literal>(&item.data))
    {
        const auto address = allocate(value_type::dict_type, item.position);
        const auto status = temporary();
        write_instruction(status + " = call i32 @txrt_dict_new(ptr " +
                          address + ")");
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
        const auto result = temporary();
        write_instruction(result + " = load ptr, ptr " + address);
        for (const auto& entry : literal->entries)
        {
            const auto key = expression_value(*entry.key);
            const auto value = expression_value(*entry.value);
            const auto boxed_key = box_any(key, entry.key->position);
            const auto boxed_value = box_any(value, entry.value->position);
            const auto set_status = temporary();
            write_instruction(set_status + " = call i32 @txrt_dict_set(ptr " +
                              result + ", ptr " + boxed_key.text + ", ptr " +
                              boxed_value.text + ")");
            write_instruction("call void @txrt_require_success(i32 " +
                              set_status + ")");
            release(boxed_key);
            release(boxed_value);
            release(key);
            release(value);
        }
        return {item.type, result};
    }
    if (const auto* name = std::get_if<name_reference>(&item.data))
    {
        return load(find_variable(name->name, item.position));
    }
    if (const auto* access = std::get_if<index_expression>(&item.data))
    {
        ir_value object{value_type::void_type, {}};
        bool borrowed_object = false;
        if (const auto* name = std::get_if<name_reference>(&access->object->data))
        {
            const auto variable = find_variable(name->name,
                                                access->object->position);
            if (is_value_handle(variable.type))
            {
                const auto handle = temporary();
                write_instruction(handle + " = load ptr, ptr " + variable.address);
                object = {variable.type, handle};
                borrowed_object = true;
            }
        }
        if (!borrowed_object)
        {
            object = expression_value(*access->object);
        }
        const auto index = expression_value(*access->index);
        const bool static_array = access->object->type == value_type::array_type;
        ir_value boxed_index{value_type::void_type, {}};
        if (!static_array)
        {
            boxed_index = box_any(index, access->index->position);
        }
        const auto address = allocate(value_type::any_type, item.position);
        const auto status = temporary();
        if (static_array)
        {
            write_instruction(status +
                " = call i32 @txrt_array_element_address(ptr " + object.text +
                ", i64 " + index.text + ", ptr " + address + ")");
        }
        else
        {
            const auto* function = access->object->type == value_type::dict_type
                ? "txrt_dict_element_address" : "txrt_value_element_address";
            write_instruction(status +
                " = call i32 @" + function + "(ptr " + object.text +
                ", ptr " + boxed_index.text + ", i1 false, ptr " + address + ")");
        }
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
        const auto borrowed = temporary();
        write_instruction(borrowed + " = load ptr, ptr " + address);
        const auto result = from_any({value_type::any_type, borrowed},
                                     item.type, item.position);
        if (!static_array)
        {
            release(boxed_index);
        }
        release(index);
        if (!borrowed_object)
        {
            release(object);
        }
        return result;
    }
    if (const auto* access = std::get_if<member_expression>(&item.data))
    {
        ir_value object{value_type::void_type, {}};
        bool borrowed_object = false;
        if (const auto* name = std::get_if<name_reference>(&access->object->data))
        {
            const auto variable = find_variable(name->name,
                                                access->object->position);
            if (is_value_handle(variable.type))
            {
                const auto handle = temporary();
                write_instruction(handle + " = load ptr, ptr " + variable.address);
                object = {variable.type, handle};
                borrowed_object = true;
            }
        }
        if (!borrowed_object)
        {
            object = expression_value(*access->object);
        }
        const auto address = allocate(value_type::any_type, item.position);
        const auto status = temporary();
        if (access->object->type == value_type::any_type)
        {
            write_instruction(status +
                " = call i32 @txrt_struct_field_address(ptr " + object.text +
                ", ptr " + global_bytes(access->field) +
                ", ptr " + address + ")");
        }
        else
        {
            const auto class_field = classes_.contains(access->object->type.name);
            if (class_field && !access->field_slot)
            {
                throw compile_error(item.position, "类字段缺少静态槽位");
            }
            write_instruction(status + " = call i32 @" +
                std::string(class_field ? "txrt_class_field_address_index"
                                        : "txrt_struct_field_address_index") +
                "(ptr " + object.text + ", i64 " +
                std::to_string(class_field ? *access->field_slot
                    : field_index(access->object->type,
                                  access->field, item.position)) +
                (class_field ? ", i1 false" : "") +
                ", ptr " + address + ")");
        }
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
        const auto borrowed = temporary();
        write_instruction(borrowed + " = load ptr, ptr " + address);
        const auto result = from_any({value_type::any_type, borrowed},
                                     item.type, item.position);
        if (!borrowed_object)
        {
            release(object);
        }
        return result;
    }
    if (const auto* operation = std::get_if<unary_operation>(&item.data))
    {
        if (operation->binding)
        {
            const auto receiver = expression_value(*operation->operand);
            return emit_operator_call(item.type, item.position,
                                      *operation->binding, receiver,
                                      std::nullopt);
        }
        if (operation->is_min_int_literal)
        {
            return {item.type, "-9223372036854775808"};
        }
        const auto value = expression_value(*operation->operand);
        if (operation->operation == token_kind::minus &&
            value.type == value_type::int_type)
        {
            return checked_unary("txrt_neg_i64", value,
                                 item.type, item.position);
        }
        const auto result = temporary();
        if (operation->operation == token_kind::minus)
        {
            write_instruction(result + " = fneg double " + value.text);
        }
        else
        {
            write_instruction(result + " = xor i1 " + value.text + ", true");
        }
        return {item.type, result};
    }
    if (const auto* operation = std::get_if<update_expression>(&item.data))
    {
        return emit_update(item, *operation);
    }
    if (const auto* operation = std::get_if<binary_operation>(&item.data))
    {
        return emit_binary(item, *operation);
    }
    if (const auto* call = std::get_if<call_expression>(&item.data))
    {
        return emit_call(item, *call);
    }
    if (const auto* cast = std::get_if<cast_expression>(&item.data))
    {
        return emit_cast(item, *cast);
    }
    throw compile_error(item.position, "LLVM 后端暂不支持此表达式");
}

} // namespace tx
