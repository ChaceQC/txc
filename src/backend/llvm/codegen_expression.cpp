#include "backend/llvm/codegen.hpp"

#include <bit>
#include <cstdint>
#include <iomanip>
#include <sstream>

namespace tx
{

std::string llvm_code_generator::decode_string_literal(std::string_view quoted)
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
        emit_array_elements(*literal, result, item.position);
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
        if (name->function_value)
        {
            const auto output = allocate(item.type, item.position);
            const auto status = temporary();
            write_instruction(status + " = call i32 @txrt_closure_new_internal(ptr " +
                callback_name(name->function_symbol, 0) + ", ptr " +
                callback_name(name->function_symbol, 0) + "_internal, ptr " +
                global_bytes(item.type.name) + ", ptr " + output + ")");
            write_instruction("call void @txrt_require_success(i32 " + status + ")");
            const auto result = temporary();
            write_instruction(result + " = load ptr, ptr " + output);
            return {item.type, result};
        }
        return load(find_variable(name->name, item.position));
    }
    if (const auto* access = std::get_if<index_expression>(&item.data))
    {
        if (access->object->type == value_type::bytes_type)
        {
            const auto object = expression_value(*access->object);
            const auto index = expression_value(*access->index);
            const auto address = allocate(value_type::int_type, item.position);
            const auto status = temporary();
            write_instruction(status + " = call i32 @txrt_bytes_at(ptr " +
                object.text + ", i64 " + index.text + ", ptr " + address + ")");
            write_instruction("call void @txrt_require_success(i32 " + status + ")");
            release(object);
            return load({value_type::int_type, address});
        }
        if (access->object->type.is_map() || access->object->type.is_deque())
        {
            return emit_map_index(*access, item.position);
        }
        if (access->object->type.is_vector())
        {
            return emit_vector_index(*access, item.position);
        }
        ir_value object{value_type::void_type, {}};
        bool borrowed_object = false;
        bool native_array = false;
        bool native_dict = false;
        if (const auto* name = std::get_if<name_reference>(&access->object->data))
        {
            const auto variable = find_variable(name->name,
                                                access->object->position);
            if (is_value_handle(variable.type))
            {
                native_array = variable.type == value_type::array_type &&
                    !variable.array_reference.empty();
                const bool simple_key =
                    std::holds_alternative<name_reference>(access->index->data) ||
                    std::holds_alternative<string_literal>(access->index->data) ||
                    std::holds_alternative<integer_literal>(access->index->data) ||
                    std::holds_alternative<floating_literal>(access->index->data) ||
                    std::holds_alternative<boolean_literal>(access->index->data) ||
                    std::holds_alternative<none_literal>(access->index->data);
                native_dict = variable.type == value_type::dict_type &&
                    !variable.dict_reference.empty() && simple_key;
                const auto handle = native_array
                    ? load_array_reference(variable) : native_dict
                    ? load_dict_reference(variable) : temporary();
                if (!native_array && !native_dict)
                {
                    write_instruction(handle + " = load ptr, ptr " +
                                      variable.address);
                }
                object = {variable.type, handle};
                borrowed_object = true;
            }
        }
        if (!borrowed_object)
        {
            object = expression_value(*access->object);
        }
        const auto* literal_key = access->object->type == value_type::dict_type
            ? std::get_if<string_literal>(&access->index->data) : nullptr;
        ir_value index{value_type::void_type, {}};
        if (!literal_key)
        {
            index = expression_value(*access->index);
        }
        const bool static_array = access->object->type == value_type::array_type;
        const bool string_key = access->object->type == value_type::dict_type &&
                                index.type == value_type::str_type;
        ir_value boxed_index{value_type::void_type, {}};
        if (!static_array && !string_key && !literal_key)
        {
            boxed_index = box_any(index, access->index->position);
        }
        std::string borrowed;
        if (static_array)
        {
            borrowed = temporary();
            write_instruction(borrowed +
                " = call ptr @" + std::string(native_array
                    ? "txrt_array_ref_element_read_ptr"
                    : "txrt_array_element_read_ptr") + "(ptr " + object.text +
                ", i64 " + index.text + ")");
        }
        else if (native_dict)
        {
            borrowed = temporary();
            if (literal_key)
            {
                const auto decoded = decode_string_literal(literal_key->text);
                write_instruction(borrowed +
                    " = call ptr @txrt_dict_ref_element_read_ptr_literal(ptr " +
                    object.text + ", ptr " + global_bytes(decoded) +
                    ", i64 " + std::to_string(decoded.size()) + ")");
            }
            else
            {
                const auto* function = string_key
                    ? "txrt_dict_ref_element_read_ptr_str"
                    : "txrt_dict_ref_element_read_ptr";
                write_instruction(borrowed + " = call ptr @" + function +
                    "(ptr " + object.text + ", ptr " +
                    (string_key ? index.text : boxed_index.text) + ")");
            }
        }
        else
        {
            const auto address = allocate(value_type::any_type, item.position, false);
            const auto status = temporary();
            if (literal_key)
            {
                const auto decoded = decode_string_literal(literal_key->text);
                write_instruction(status +
                    " = call i32 @txrt_dict_element_address_literal(ptr " +
                    object.text + ", ptr " + global_bytes(decoded) +
                    ", i64 " + std::to_string(decoded.size()) +
                    ", i1 false, ptr " + address + ")");
            }
            else
            {
                const auto* function = string_key
                    ? "txrt_dict_element_address_str" :
                    access->object->type == value_type::dict_type
                    ? "txrt_dict_element_address" :
                    "txrt_value_element_address";
                write_instruction(status +
                    " = call i32 @" + function + "(ptr " + object.text +
                    ", ptr " + (string_key ? index.text : boxed_index.text) +
                    ", i1 false, ptr " + address + ")");
            }
            write_instruction("call void @txrt_require_success(i32 " + status +
                              ")");
            borrowed = temporary();
            write_instruction(borrowed + " = load ptr, ptr " + address);
        }
        const auto result = from_any({value_type::any_type, borrowed},
                                     item.type, item.position);
        if (!static_array && !string_key && !literal_key)
        {
            release(boxed_index);
        }
        if (!literal_key)
        {
            release(index);
        }
        if (!borrowed_object)
        {
            release(object);
        }
        return result;
    }
    if (const auto* access = std::get_if<member_expression>(&item.data))
    {
        if (direct_scalar_field(item))
        {
            return load({item.type, scalar_field_address(item, true)});
        }
        ir_value object{value_type::void_type, {}};
        bool borrowed_object = false;
        if (const auto* name = std::get_if<name_reference>(&access->object->data))
        {
            const auto variable = find_variable(name->name,
                                                access->object->position);
            if (is_value_handle(variable.type))
            {
                materialize_native_parse(variable);
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
        if (static_record_type(access->object->type))
        {
            const auto result = read_record_field(object, item, *access);
            if (!borrowed_object)
            {
                release(object);
            }
            return result;
        }
        if (access->object->type == value_type::any_type)
        {
            const auto output = allocate(value_type::any_type, item.position);
            const auto status = temporary();
            write_instruction(status + " = call i32 @txrt_struct_field_get(ptr " + object.text +
                              ", ptr " + global_bytes(access->field) + ", ptr " + output + ")");
            write_instruction("call void @txrt_require_success(i32 " + status + ")");
            const auto result = temporary();
            write_instruction(result + " = load ptr, ptr " + output);
            if (!borrowed_object)
            {
                release(object);
            }
            return {item.type, result};
        }
        const auto address = allocate(value_type::any_type, item.position, false);
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
        if (operation->operation == token_kind::keyword_await)
        {
            return emit_task_wait(item, expression_value(*operation->operand));
        }
        if (operation->binding)
        {
            bool borrowed = false;
            const auto receiver = expression_value_or_borrow(
                *operation->operand, borrowed);
            return emit_operator_call(item.type, item.position,
                                      *operation->binding, receiver,
                                      std::nullopt, borrowed);
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
        if (const auto* index = std::get_if<index_expression>(&operation->target->data);
            index && (index->object->type.is_map() ||
                      index->object->type.is_deque()))
        {
            return emit_map_update(item, *operation);
        }
        if (const auto* index = std::get_if<index_expression>(&operation->target->data);
            index && index->object->type.is_vector())
        {
            return emit_vector_update(item, *operation);
        }
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
