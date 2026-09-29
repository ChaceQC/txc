#include "backend/llvm/codegen.hpp"

#include <algorithm>

namespace tx
{

std::string llvm_code_generator::container_symbol(const value_type& type,
                                                   std::string_view operation)
{
    std::string symbol = "txrt_" + type.container_name() + "_" + std::string(operation);
    for (const auto& parameter : type.parameters)
    {
        const bool object_element = (type.is_deque() || type.is_map() ||
            type.is_priority_entry() ||
            type.container_name() == "heap" ||
            type.container_name() == "queue" ||
            type.container_name() == "ordered_set") &&
            parameter != value_type::int_type && parameter != value_type::float_type &&
            parameter != value_type::bool_type && parameter != value_type::str_type;
        symbol += "_" + (object_element ? "object" :
            vector_suffix(value_type::vector_of(parameter)));
    }
    return symbol;
}

llvm_code_generator::ir_value llvm_code_generator::container_operation(
    const value_type& type, std::string_view operation,
    const std::vector<ir_value>& arguments, const value_type& result_type, source_pos position)
{
    std::string parameters;
    const auto& key = type.parameters.front();
    const bool object_key = (type.container_name() == "map" ||
        type.container_name() == "set") &&
        key != value_type::int_type && key != value_type::float_type &&
        key != value_type::bool_type && key != value_type::str_type;
    if (object_key && operation == "new")
    {
        parameters = "ptr " + global_bytes(key.name) + ", ptr " +
            key_hash_symbol(key) + ", ptr " + key_equal_symbol(key);
    }
    if ((type.container_name() == "ordered_map" ||
         type.container_name() == "ordered_set") && operation == "new")
    {
        parameters = "ptr " + global_bytes(key.name);
        if (type.container_name() == "ordered_map")
        {
            parameters += ", ptr " + global_bytes(type.parameters[1].name);
        }
        const bool structural_key = key != value_type::int_type &&
            key != value_type::float_type && key != value_type::bool_type &&
            key != value_type::str_type;
        parameters += ", ptr " + (structural_key && arguments.empty()
            ? key_less_symbol(key) : std::string("null"));
        if (arguments.empty())
        {
            parameters += ", ptr null";
        }
    }
    if ((type.is_deque() || type.container_name() == "map") && operation == "new" &&
        key != value_type::int_type && key != value_type::float_type &&
        key != value_type::bool_type && key != value_type::str_type &&
        type.is_deque())
    {
        parameters = "ptr " + global_bytes(key.name);
    }
    if (type.is_priority_entry() && operation == "new")
    {
        parameters = "ptr " + global_bytes(type.name);
    }
    const auto kind = type.container_name();
    const bool object_sequence = (kind == "heap" || kind == "queue") &&
        container_symbol(type, "new").ends_with("_object");
    const bool special_sequence =
        ((kind == "heap" || kind == "queue") && operation == "build") ||
        (object_sequence && operation == "new");
    if (special_sequence)
    {
        const bool explicit_compare = kind == "heap" && operation == "build" &&
            arguments[1].text != "null";
        const auto less = structs_.contains(key.name) && !explicit_compare
            ? key_less_symbol(key) : std::string("null");
        if (kind == "heap" && operation == "build")
        {
            parameters = "i1 " + arguments[0].text + ", ptr " +
                global_bytes(key.name) + ", ptr " + less + ", ptr " +
                arguments[1].text + ", ptr " + arguments[2].text;
        }
        else if (kind == "heap")
        {
            parameters = "i1 " + arguments[0].text + ", ptr " +
                global_bytes(key.name) + ", ptr " + less;
        }
        else if (operation == "build")
        {
            parameters = "ptr " + global_bytes(key.name) + ", ptr " +
                arguments[0].text;
        }
        else
        {
            parameters = "ptr " + global_bytes(key.name);
        }
    }
    if (type.container_name() == "map" && operation == "new" &&
        type.parameters[1] != value_type::int_type &&
        type.parameters[1] != value_type::float_type &&
        type.parameters[1] != value_type::bool_type &&
        type.parameters[1] != value_type::str_type)
    {
        parameters += (parameters.empty() ? "" : ", ") +
            std::string("ptr ") + global_bytes(type.parameters[1].name);
    }
    for (const auto& argument : arguments)
    {
        if (special_sequence)
        {
            break;
        }
        parameters += (parameters.empty() ? "" : ", ") +
                      llvm_type(argument.type, position) + " " + argument.text;
    }
    const bool returns_value = result_type != value_type::void_type;
    const auto output = returns_value ? allocate(result_type, position) : "";
    if (returns_value)
    {
        parameters += (parameters.empty() ? "" : ", ") + std::string("ptr ") + output;
    }
    const auto status = temporary();
    write_instruction(status + " = call i32 @" + container_symbol(type, operation) +
                      "(" + parameters + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    if (!returns_value)
    {
        return {value_type::void_type, {}};
    }
    const auto result = temporary();
    write_instruction(result + " = load " + llvm_type(result_type, position) + ", ptr " + output);
    return {result_type, result};
}

llvm_code_generator::ir_value llvm_code_generator::emit_container_call(
    const expression& item, const call_expression& call)
{
    const auto& type = call.container_type ? *call.container_type : call.receiver->type;
    std::vector<ir_value> arguments;
    if (call.receiver)
    {
        const bool stable = std::all_of(call.arguments.begin(), call.arguments.end(),
            [](const call_argument& argument)
            {
                return stable_value_expression(*argument.value);
            });
        bool borrowed = false;
        auto receiver = container_value(*call.receiver,
            (call.properties.receiver == argument_ownership::borrowed ||
             default_heap_receiver(call)) && stable, borrowed);
        // 不跨可能重绑定字段的实参或用户回调借用；不安全时仍先持有接收者。
        receiver.borrowed = borrowed;
        arguments.push_back(receiver);
    }
    for (const auto& argument : call.arguments)
    {
        arguments.push_back(expression_value(*argument.value));
    }
    std::string operation = call.container_type ? "new" : call.name;
    std::vector<ir_value> selected = arguments;
    if (call.container_type && type.container_name() == "heap")
    {
        ir_value descending{value_type::bool_type, "false"};
        ir_value comparator{value_type::any_type, "null"};
        ir_value input{value_type::any_type, "null"};
        for (const auto& argument : arguments)
        {
            if (argument.type == value_type::bool_type)
            {
                descending = argument;
            }
            else if (argument.type.is_vector())
            {
                input = argument;
            }
            else
            {
                comparator = argument;
            }
        }
        if (comparator.text != "null" || input.text != "null")
        {
            operation = "build";
            selected = {descending, comparator, input};
        }
        else
        {
            selected = {descending};
        }
    }
    if (call.container_type && type.container_name() == "queue" &&
        !arguments.empty())
    {
        operation = "build";
        selected = {arguments.front()};
    }
    const auto result = container_operation(type, operation,
                                            selected, item.type, item.position);
    for (const auto& argument : arguments)
    {
        release(argument);
    }
    return result;
}

void llvm_code_generator::initialize_container_fields(
    const class_decl& definition, const std::string& object, source_pos position)
{
    std::vector<const class_decl*> nodes;
    collect_class_nodes(definition, nodes);
    for (const auto* node : nodes)
    {
        for (const auto& field : node->fields)
        {
            if (!field.type.is_typed_container())
            {
                continue;
            }
            std::vector<ir_value> arguments;
            if (field.type.container_name() == "heap")
            {
                arguments.push_back({value_type::bool_type, "false"});
            }
            // 字段默认值同样在编译期选定构造入口，不按类型名称在运行时分派。
            const auto value = container_operation(field.type, "new", arguments, field.type, position);
            const auto address = allocate(value_type::any_type, position, false);
            const auto status = temporary();
            write_instruction(status + " = call i32 @txrt_class_field_address_index(ptr " +
                object + ", i64 " + std::to_string(field.slot) + ", i1 true, ptr " + address + ")");
            write_instruction("call void @txrt_require_success(i32 " + status + ")");
            const auto slot = temporary();
            write_instruction(slot + " = load ptr, ptr " + address);
            assign_any(slot, value, position);
            release(value);
        }
    }
}

} // namespace tx
