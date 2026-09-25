#include "backend/llvm/codegen.hpp"

#include <algorithm>

namespace tx
{
namespace
{

bool supported_external_call(std::string_view name)
{
    return name == "io.write" || name == "io.write_line" ||
           name == "io.write_error" || name == "io.flush" ||
           name == "string.contains" || name == "string.starts_with" ||
           name == "string.ends_with" || name == "string.find" ||
           name == "string.slice" || name == "string.replace" ||
           name == "string.split" || name == "string.join" ||
           name == "string.trim" || name == "string.lower" ||
           name == "string.upper" || name == "format.format" ||
           name == "file.read_text" ||
           name == "file.write_text" || name == "file.append_text" ||
           name == "math.abs" || name == "math.min" ||
           name == "math.max" || name == "math.clamp" ||
           name == "math.mod" || name == "math.sqrt" ||
           name == "math.pow" || name == "math.floor" ||
           name == "math.ceil" || name == "array.concat" ||
           name == "array.slice" || name == "array.reverse" ||
           name == "fs.exists" || name == "fs.is_file" ||
           name == "fs.is_directory" ||
           name == "fs.create_directories" ||
           name == "fs.list_directory" ||
           name == "path.join" || name == "path.parent" ||
           name == "path.file_name" || name == "path.extension" ||
           name == "time.unix_millis" ||
           name == "time.monotonic_millis" ||
           name == "time.monotonic_micros" ||
           name == "time.sleep_millis" || name == "random.seed" ||
           name == "random.random_int" ||
           name == "random.random_float";
}

bool is_builtin_call(std::string_view name)
{
    return name == "print" || name == "len" || name == "is_none" ||
           name == "to_float" || name == "input" ||
           name == "input_or_none" || name == "deep_copy";
}

} // namespace

llvm_code_generator::ir_value llvm_code_generator::emit_builtin_call(
    const expression& item, const call_expression& call,
    const std::vector<ir_value>& arguments, bool borrowed_argument)
{
    if (call.name == "print")
    {
        emit_print_call(item, call, arguments);
        return {value_type::void_type, {}};
    }
    if (call.name == "deep_copy")
    {
        const auto& argument = arguments.front();
        if (!is_value_handle(argument.type))
        {
            return argument;
        }
        const auto address = allocate(argument.type, item.position);
        const auto status = temporary();
        write_instruction(status + " = call i32 @txrt_value_deep_copy(ptr " +
                          argument.text + ", ptr " + address + ")");
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
        release(argument);
        const auto result = temporary();
        write_instruction(result + " = load ptr, ptr " + address);
        return {item.type, result};
    }
    if (call.name == "len")
    {
        const auto& argument = arguments.front();
        const auto* name = argument.type == value_type::str_type
            ? "txrt_str_len" : argument.type == value_type::array_type
            ? "txrt_array_len" : argument.type == value_type::dict_type
            ? "txrt_dict_len" : argument.type == value_type::any_type
            ? "txrt_value_len" : nullptr;
        if (!name)
        {
            throw compile_error(item.position, "LLVM 后端暂不支持此 len 类型");
        }
        const auto address = allocate(value_type::int_type, item.position);
        const auto status = temporary();
        write_instruction(status + " = call i32 @" + name + "(ptr " +
                          argument.text + ", ptr " + address + ")");
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
        if (!borrowed_argument)
        {
            release(argument);
        }
        return load({value_type::int_type, address});
    }
    if (call.name == "is_none")
    {
        const auto address = allocate(value_type::bool_type, item.position);
        const auto status = temporary();
        write_instruction(status + " = call i32 @txrt_value_is_none(ptr " +
                          arguments.front().text + ", ptr " + address + ")");
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
        if (!borrowed_argument)
        {
            release(arguments.front());
        }
        return load({value_type::bool_type, address});
    }
    if (call.name == "to_float")
    {
        const auto result = temporary();
        write_instruction(result + " = sitofp i64 " + arguments.front().text +
                          " to double");
        return {value_type::float_type, result};
    }
    const auto result_type = call.name == "input" ? value_type::str_type
                                                   : value_type::any_type;
    const auto address = allocate(result_type, item.position);
    const auto status = temporary();
    const auto prompt = arguments.empty() ? "null" : arguments.front().text;
    const auto* name = call.name == "input" ? "txrt_input" : "txrt_input_or_none";
    write_instruction(status + " = call i32 @" + name + "(ptr " +
                      prompt + ", ptr " + address + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    if (!arguments.empty())
    {
        release(arguments.front());
    }
    const auto result = temporary();
    write_instruction(result + " = load ptr, ptr " + address);
    return {result_type, result};
}

llvm_code_generator::ir_value llvm_code_generator::emit_constructor_call(
    const expression& item, const call_expression& call,
    const std::vector<ir_value>& arguments)
{
    const auto found = structs_.find(call.name);
    if (found == structs_.end())
    {
        throw compile_error(item.position, "LLVM 后端找不到结构体：" + call.name);
    }
    const auto& fields = found->second->fields;
    const auto separator = call.source_name.find_last_of('.');
    const auto display_name = call.source_name.substr(
        separator == std::string::npos ? 0 : separator + 1);
    const auto address = allocate(item.type, item.position);
    const auto status = temporary();
    write_instruction(status + " = call i32 @txrt_struct_new(ptr " +
                      global_bytes(call.name) + ", ptr " +
                      global_bytes(display_name) + ", i64 " +
                      std::to_string(fields.size()) + ", ptr " + address + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    const auto result = temporary();
    write_instruction(result + " = load ptr, ptr " + address);
    for (std::size_t index = 0; index < fields.size(); ++index)
    {
        const auto& argument = arguments[index];
        const auto* direct = argument.type == value_type::int_type
            ? "txrt_struct_set_field_i64" :
            argument.type == value_type::float_type
            ? "txrt_struct_set_field_f64" :
            argument.type == value_type::bool_type
            ? "txrt_struct_set_field_bool" : nullptr;
        ir_value boxed{value_type::void_type, {}};
        if (!direct)
        {
            boxed = box_any(argument, item.position);
        }
        const auto set_status = temporary();
        write_instruction(set_status + " = call i32 @" +
            std::string(direct ? direct : "txrt_struct_set_field") +
            "(ptr " + result + ", i64 " + std::to_string(index) +
            ", ptr " + global_bytes(fields[index].name) + ", " +
            (direct ? llvm_type(argument.type, item.position) + " " +
            argument.text : "ptr " + boxed.text) + ")");
        write_instruction("call void @txrt_require_success(i32 " +
                          set_status + ")");
        release(boxed);
        release(arguments[index]);
    }
    return {item.type, result};
}

llvm_code_generator::ir_value llvm_code_generator::emit_external_call(
    const expression& item, const call_expression& call,
    const function_decl& target, const std::vector<ir_value>& arguments)
{
    if (!supported_external_call(target.external_name))
    {
        throw compile_error(item.position, "标准库没有此二进制函数：" +
                                           call.source_name);
    }
    return emit_direct_external_call(item, target, arguments);
}

llvm_code_generator::ir_value llvm_code_generator::emit_user_call(
    const expression& item, const call_expression& call,
    const function_decl& target, const std::vector<ir_value>& arguments)
{
    std::string arguments_text;
    for (std::size_t index = 0; index < arguments.size(); ++index)
    {
        if (index != 0)
        {
            arguments_text += ", ";
        }
        arguments_text += llvm_type(arguments[index].type, item.position) +
                          " " + arguments[index].text;
    }
    const auto invocation = "call " + llvm_type(target.return_type, item.position) +
        " " + function_name(call.name, *call.overload_index) +
        "(" + arguments_text + ")";
    if (target.return_type == value_type::void_type)
    {
        write_instruction(invocation);
        return {value_type::void_type, {}};
    }
    const auto result = temporary();
    write_instruction(result + " = " + invocation);
    return {item.type, result};
}

llvm_code_generator::ir_value llvm_code_generator::emit_call(
    const expression& item, const call_expression& call)
{
    if (call.is_super_view)
    {
        const auto value = load(find_variable("self", item.position));
        return {item.type, value.text};
    }
    if (call.receiver)
    {
        return emit_method_call(item, call);
    }
    const auto plain_arguments = [&]
    {
        std::vector<ir_value> values;
        values.reserve(call.arguments.size());
        for (const auto& argument : call.arguments)
        {
            values.push_back(expression_value(*argument.value));
        }
        return values;
    };
    if (is_builtin_call(call.name))
    {
        if (call.name == "len" || call.name == "is_none")
        {
            bool borrowed = false;
            const auto& input = *call.arguments.front().value;
            if (call.name == "len" && input.type == value_type::array_type)
            {
                if (const auto* name = std::get_if<name_reference>(&input.data))
                {
                    const auto slot = find_variable(name->name, input.position);
                    if (has_local_array(slot))
                    {
                        return {value_type::int_type,
                            slot.local_array_length
                                ? std::to_string(*slot.local_array_length)
                                : slot.dynamic_array_length};
                    }
                    if (!slot.array_reference.empty())
                    {
                        const auto reference = load_array_reference(slot);
                        const auto length = temporary();
                        write_instruction(length +
                            " = call i64 @txrt_array_ref_len(ptr " +
                            reference + ")");
                        return {value_type::int_type, length};
                    }
                }
            }
            ir_value argument{value_type::void_type, {}};
            if (call.name == "len" && input.type == value_type::str_type)
            {
                if (const auto* name = std::get_if<name_reference>(&input.data))
                {
                    const auto slot = find_variable(name->name, input.position);
                    const auto text = temporary();
                    write_instruction(text + " = load ptr, ptr " + slot.address);
                    argument = {input.type, text};
                    borrowed = true;
                }
            }
            if (const auto* index = std::get_if<index_expression>(&input.data);
                index && input.type == value_type::any_type &&
                index->object->type == value_type::array_type)
            {
                if (const auto* owner = std::get_if<name_reference>(
                        &index->object->data))
                {
                    // 索引先求值，再从变量槽借用元素；内置函数不会保存地址。
                    const auto position = expression_value(*index->index);
                    const auto slot = find_variable(owner->name, input.position);
                    if (has_local_array(slot))
                    {
                        const auto [kind_address, bits_address] =
                            local_array_element(slot, position.text, false);
                        const auto kind = temporary();
                        write_instruction(kind + " = load i8, ptr " + kind_address);
                        const auto result = temporary();
                        write_instruction(result + " = icmp eq i8 " + kind +
                                          ", 1");
                        release(position);
                        (void)bits_address;
                        return {value_type::bool_type, result};
                    }
                    const bool native = !slot.array_reference.empty();
                    const auto array = native
                        ? load_array_reference(slot) : temporary();
                    if (!native)
                    {
                        write_instruction(array + " = load ptr, ptr " +
                                          slot.address);
                    }
                    const auto element = temporary();
                    write_instruction(element +
                        " = call ptr @" + std::string(native
                            ? "txrt_array_ref_element_read_ptr"
                            : "txrt_array_element_read_ptr") + "(ptr " + array +
                        ", i64 " + position.text + ")");
                    release(position);
                    argument = {input.type, element};
                    borrowed = true;
                }
            }
            if (!borrowed)
            {
                argument = expression_value_or_borrow(input, borrowed);
            }
            return emit_builtin_call(item, call, {argument}, borrowed);
        }
        return emit_builtin_call(item, call, plain_arguments());
    }
    if (call.is_constructor)
    {
        if (const auto found_class = classes_.find(call.name);
            found_class != classes_.end())
        {
            const function_decl* init = call.constructor_init_symbol.empty()
                ? nullptr
                : functions_.at(call.constructor_init_symbol).at(
                    call.constructor_init_index);
            const bool needs_binding = init != nullptr &&
                (std::any_of(init->parameters.begin(), init->parameters.end(),
                    [](const parameter& parameter)
                    { return parameter.kind != parameter_kind::ordinary; }) ||
                 std::any_of(call.arguments.begin(), call.arguments.end(),
                    [](const call_argument& argument)
                    { return argument.kind != argument_kind::positional; }));
            std::vector<ir_value> arguments;
            std::vector<bool> borrowed_arguments;
            if (needs_binding)
            {
                arguments = emit_bound_arguments(item, call, init->parameters);
                borrowed_arguments.resize(arguments.size(), false);
            }
            else
            {
                arguments.reserve(call.arguments.size());
                for (std::size_t index = 0; index < call.arguments.size(); ++index)
                {
                    bool borrowed = false;
                    const auto& argument = *call.arguments[index].value;
                    arguments.push_back(init &&
                        init_parameter_borrowed(*init, index)
                        ? expression_value_or_borrow(argument, borrowed)
                        : expression_value(argument));
                    borrowed_arguments.push_back(borrowed);
                }
            }
            return emit_class_constructor(item, call, arguments,
                                          borrowed_arguments);
        }
        const auto& definition = *structs_.at(call.name);
        std::vector<parameter> parameters;
        for (const auto& field : definition.fields)
        {
            parameters.push_back({field.name, field.type, field.position});
        }
        const bool needs_binding = std::any_of(call.arguments.begin(),
            call.arguments.end(), [](const call_argument& argument)
            { return argument.kind != argument_kind::positional; });
        const auto arguments = needs_binding
            ? emit_bound_arguments(item, call, parameters) : plain_arguments();
        return emit_constructor_call(item, call, arguments);
    }
    if (!call.overload_index)
    {
        throw compile_error(item.position, "LLVM 后端暂不支持此调用");
    }
    const auto found = functions_.find(call.name);
    if (found == functions_.end() ||
        *call.overload_index >= found->second.size())
    {
        throw compile_error(item.position, "LLVM 后端找不到函数调用目标");
    }
    const auto& target = *found->second[*call.overload_index];
    const bool needs_binding = std::any_of(target.parameters.begin(),
        target.parameters.end(), [](const parameter& value)
        { return value.kind != parameter_kind::ordinary; }) ||
        std::any_of(call.arguments.begin(), call.arguments.end(),
            [](const call_argument& value)
            { return value.kind != argument_kind::positional; });
    if (target.external)
    {
        const auto arguments = needs_binding
            ? emit_bound_arguments(item, call, target.parameters)
            : plain_arguments();
        return emit_external_call(item, call, target, arguments);
    }
    std::vector<ir_value> arguments;
    std::vector<bool> borrowed_arguments;
    if (needs_binding)
    {
        arguments = emit_bound_arguments(item, call, target.parameters);
        borrowed_arguments.resize(arguments.size(), false);
    }
    else
    {
        arguments.reserve(call.arguments.size());
        for (std::size_t index = 0; index < call.arguments.size(); ++index)
        {
            bool borrowed = false;
            const auto& argument = *call.arguments[index].value;
            arguments.push_back(ordinary_parameter_borrowed(target, index)
                ? expression_value_or_borrow(argument, borrowed)
                : expression_value(argument));
            borrowed_arguments.push_back(borrowed);
        }
    }
    const auto result = emit_user_call(item, call, target, arguments);
    for (std::size_t index = 0; index < arguments.size(); ++index)
    {
        if (ordinary_parameter_borrowed(target, index) &&
            !borrowed_arguments[index])
        {
            release(arguments[index]);
        }
    }
    return result;
}

} // namespace tx
