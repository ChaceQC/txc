#include "backend/llvm/codegen.hpp"

#include <algorithm>

namespace tx
{
namespace
{

bool supported_external_call(std::string_view name)
{
    return name.starts_with("graphics.") || name.starts_with("gui.") ||
           name.starts_with("httpx.") || name.starts_with("websocket.") ||
           name.starts_with("dns.") || name.starts_with("socket.") ||
           name.starts_with("thread.") || name.starts_with("sync.") ||
           name.starts_with("channel.") || name.starts_with("task.") ||
           name.starts_with("async_file.") ||
           name.starts_with("ipc.") || name.starts_with("db.") ||
           name.starts_with("unicode.") ||
           name.starts_with("regex.") ||
           name.starts_with("crypto.") || name.starts_with("secret.") ||
           name.starts_with("password.") ||
           name.starts_with("public_key.") || name.starts_with("x509.") ||
           name.starts_with("tls.") ||
           name.starts_with("test.") || name.starts_with("log.") ||
           name.starts_with("debug.") || name.starts_with("profile.") ||
           name == "error.fail_io" || name == "error.stack_trace" ||
           name.starts_with("cancel.") ||
           name.starts_with("process.") ||
           name.starts_with("json.") ||
           name.starts_with("cbor.") ||
           name.starts_with("csv.") ||
           name.starts_with("xml.") ||
           name.starts_with("time.") ||
           name == "io.write" || name == "io.write_line" ||
           name == "io.write_error" || name == "io.flush" ||
           name == "string.contains" || name == "string.starts_with" ||
           name == "string.ends_with" || name == "string.find" ||
           name == "string.slice" || name == "string.replace" ||
           name == "string.split" || name == "string.join" ||
           name == "string.split_vector" ||
           name == "string.trim" || name == "string.lower" ||
           name == "string.upper" || name == "format.format" ||
           name == "parse.try_parse_int" || name == "parse.try_parse_float" ||
           name == "parse.parse_int" || name == "parse.parse_float" ||
           name == "json.parse" || name == "json.parse_object" ||
           name == "json.try_parse" ||
           name == "json.stringify" || name == "json.stringify_pretty" ||
           name == "json.contains" || name == "json.get" ||
           name == "json.get_int" || name == "json.get_float" ||
           name == "json.get_bool" || name == "json.get_str" ||
           name == "json.get_array" || name == "json.get_object" ||
           name == "file.try_read_text" || name == "file.try_write_text" ||
           name == "file.try_append_text" || name == "file.read_text" ||
           name == "file.write_text" || name == "file.append_text" ||
           name == "file.read_bytes" || name == "file.write_bytes" ||
           name == "file.atomic_write_bytes" ||
           name == "file.atomic_write_text" ||
           name == "bytes.empty" || name == "bytes.from_vector" ||
           name == "bytes.to_vector" || name == "bytes.concat" ||
           name == "bytes.slice" || name == "bytes.to_hex" ||
           name == "bytes.from_hex" || name == "bytes.to_base64" ||
           name == "bytes.from_base64" ||
           name == "bytes.to_base64_url" ||
           name == "bytes.from_base64_url" ||
           name == "bytes.to_hex_chunk" ||
           name == "bytes.to_base64_chunk" ||
           name.starts_with("encoding.") ||
           name == "file_stream.open_binary" ||
           name == "file_stream.open_binary_shared" ||
           name == "file_stream.read_bytes" ||
           name == "file_stream.read_all_bytes" ||
           name == "file_stream.write_bytes" ||
           name == "file_stream.tell" || name == "file_stream.seek" ||
           name == "file_stream.flush" || name == "file_stream.close" ||
           name == "file_stream.sync" || name == "file_stream.lock" ||
           name == "file_stream.try_lock" ||
           name == "file_stream.unlock" ||
           name == "file_stream.open_text" ||
           name == "file_stream.read_chars" ||
           name == "file_stream.read_line" ||
           name == "file_stream.write_text" ||
           name == "file_stream.write_line" ||
           name.starts_with("math.") || name.starts_with("statistics.") ||
           name.starts_with("decimal.") ||
           name == "array.concat" ||
           name == "algorithm.sort" || name == "algorithm.sorted" ||
           name == "algorithm.find" || name == "algorithm.count" ||
           name == "algorithm.lower_bound" || name == "algorithm.upper_bound" ||
           name == "algorithm.reverse" || name == "algorithm.sum" ||
           name == "algorithm.min_element" || name == "algorithm.max_element" ||
           name == "array.slice" || name == "array.reverse" ||
           name == "array.push_back" || name == "array.pop_back" ||
           name == "array.insert" || name == "array.erase" ||
           name == "array.clear" ||
           name == "dictionary.get" || name == "dictionary.contains" ||
           name == "dictionary.remove" || name == "dictionary.keys" ||
           name == "dictionary.values" || name == "dictionary.clear" ||
           name == "dictionary.items" ||
           name.starts_with("fs.") || name.starts_with("path.") ||
           name == "system.args" || name == "system.current_directory" ||
           name == "system.set_current_directory" ||
           name == "system.executable_path" || name == "system.temp_directory" ||
           name == "system.home_directory" ||
           name == "system.operating_system" ||
           name == "system.architecture" ||
           name == "system.cpu_count" ||
           name == "system.process_id" ||
           name == "system.has_capability" ||
           name == "env.contains" || name == "env.get" ||
           name == "env.set" || name == "env.remove" ||
           name == "env.snapshot" ||
           name == "time.unix_millis" ||
           name == "time.monotonic_millis" ||
           name == "time.monotonic_micros" ||
           name == "time.sleep_millis" || name.starts_with("random.");
}

bool is_builtin_call(std::string_view name)
{
    return name == "print" || name == "len" || name == "is_none" ||
           name == "to_float" || name == "input" ||
           name == "input_or_none" || name == "deep_copy" ||
           name == "assert_send" || name == "assert_sync" ||
           name == "move";
}

bool simple_argument(const call_argument& argument)
{
    const auto& value = argument.value->data;
    return std::holds_alternative<name_reference>(value) ||
           std::holds_alternative<string_literal>(value) ||
           std::holds_alternative<integer_literal>(value) ||
           std::holds_alternative<floating_literal>(value) ||
           std::holds_alternative<boolean_literal>(value) ||
           std::holds_alternative<none_literal>(value);
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
    if (call.name == "assert_send" || call.name == "assert_sync")
    {
        release(arguments.front());
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
        write_instruction(status + " = call i32 @txrt_value_deep_copy_known(ptr " +
                          argument.text + ", i64 " + std::to_string(record_copy_tag(argument.type)) +
                          ", ptr " + address + ")");
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
            ? "txrt_str_len" : argument.type == value_type::bytes_type
            ? "txrt_value_len" : argument.type == value_type::array_type
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
    if (static_record_type(item.type))
    {
        return emit_record_constructor(item, arguments);
    }
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
    if (target.external_name.starts_with("thread.") &&
        target.external_name != "thread.take_detached_error")
    {
        return emit_thread_intrinsic(item, target, arguments);
    }
    if (target.external_name.starts_with("task.") &&
        !target.type_parameters.empty())
    {
        return emit_task_intrinsic(item, target, arguments);
    }
    if (target.external_name.starts_with("sync.") &&
        !target.type_parameters.empty())
    {
        return emit_sync_intrinsic(item, target, arguments);
    }
    if (target.external_name.starts_with("channel."))
    {
        return emit_channel_intrinsic(item, target, arguments);
    }
    if (target.external_name.starts_with("serde."))
    {
        return emit_serde_intrinsic(item, target, arguments);
    }
    if (target.external_name.starts_with("algorithm.") &&
        target.external_name != "algorithm.sort" &&
        target.external_name != "algorithm.sorted" &&
        target.external_name != "algorithm.find" &&
        target.external_name != "algorithm.count" &&
        target.external_name != "algorithm.lower_bound" &&
        target.external_name != "algorithm.upper_bound" &&
        target.external_name != "algorithm.reverse" &&
        target.external_name != "algorithm.sum" &&
        target.external_name != "algorithm.min_element" &&
        target.external_name != "algorithm.max_element")
    {
        return emit_algorithm_intrinsic(item, target, arguments);
    }
    if (target.external_name.starts_with("requests."))
    {
        const auto symbol = "m0_bridge_" + target.external_name.substr(9);
        const auto result = emit_user_call(item, call, target, arguments, symbol);
        if (!recoverable_errors_)
        {
            const auto status = temporary();
            write_instruction(status + " = load i32, ptr %tx_error_kind");
            write_instruction("call void @txrt_require_success(i32 " + status + ")");
        }
        for (std::size_t index = 0; index < arguments.size(); ++index)
        {
            if (ordinary_parameter_borrowed(target, index))
            {
                release(arguments[index]);
            }
        }
        return result;
    }
    if (!supported_external_call(target.external_name))
    {
        throw compile_error(item.position, "标准库没有此二进制函数：" +
                                           call.source_name);
    }
    return emit_direct_external_call(item, target, arguments);
}

llvm_code_generator::ir_value llvm_code_generator::emit_user_call(
    const expression& item, const call_expression& call,
    const function_decl& target, const std::vector<ir_value>& arguments,
    std::string_view symbol_override, std::string_view receiver_view)
{
    const auto passed = coerce_nullable_arguments(target, arguments);
    std::string arguments_text = "ptr %tx_context";
    for (std::size_t index = 0; index < passed.size(); ++index)
    {
        arguments_text += ", ";
        arguments_text += llvm_type(passed[index].type, item.position) +
                          " " + passed[index].text;
        if (index == 0 && !receiver_view.empty())
        {
            arguments_text += ", ptr " + std::string(receiver_view);
        }
    }
    const auto symbol = symbol_override.empty() ? call.name
        : std::string(symbol_override);
    const auto proven_result = bounded_integer_result(target);
    const auto argument_range = passed.size() == 1 ? integer_range_of(passed.front()) : integer_interval{-1, -1};
    const bool bounded = proven_result && passed.size() == 1 &&
        (proven_integer_function_ == &target || (argument_range.first >= 0 && argument_range.second <= 256));
    const auto invocation = "call " + llvm_type(target.return_type, item.position) +
        " " + function_name(symbol, *call.overload_index) +
        (bounded ? "_bounded" : receiver_view.empty() ? "" : "_record") +
        "(" + arguments_text + ")";
    transfer_call_arguments(target, passed);
    emit_stack_location();
    if (target.return_type == value_type::void_type)
    {
        write_instruction(invocation);
        return {value_type::void_type, {}};
    }
    const auto result = temporary();
    if (bounded)
    {
        emitted_bounded_calls_.insert(&item);
        body_ << "  " << result << " = " << invocation << '\n';
    }
    else
    {
        write_instruction(result + " = " + invocation);
    }
    auto value = own_direct_value({item.type, result});
    value.integer_range = bounded ? proven_result : constant_integer_result(target);
    return value;
}

llvm_code_generator::ir_value llvm_code_generator::emit_callback_call(
    const expression& item, const call_expression& call)
{
    const auto& variable = find_variable(call.source_name, item.position);
    // 简单实参不会改写调用目标；局部变量在同步调用结束前持续持有闭包。
    const bool borrow_closure = !variable.borrowed &&
        std::all_of(call.arguments.begin(), call.arguments.end(), simple_argument);
    ir_value closure{variable.type, {}};
    if (borrow_closure)
    {
        const auto handle = temporary();
        write_instruction(handle + " = load ptr, ptr " + variable.address);
        closure.text = handle;
    }
    else
    {
        closure = load(variable);
    }
    auto view = variable.closure_view;
    if (view.empty())
    {
        view = temporary();
        write_instruction(view + " = call ptr @txrt_closure_view(ptr " + closure.text + ")");
    }
    auto pointer = variable.closure_target;
    if (pointer.empty())
    {
        pointer = temporary();
        write_instruction(pointer + " = load ptr, ptr " + view);
    }
    std::string parameters = "ptr %tx_context, ptr " + view;
    std::vector<ir_value> arguments;
    for (const auto& argument : call.arguments)
    {
        arguments.push_back(expression_value(*argument.value));
        parameters += ", " + llvm_type(arguments.back().type, argument.position) +
                      " " + arguments.back().text;
    }
    const auto invocation = "call " + llvm_type(item.type, item.position) +
        " " + pointer + "(" + parameters + ")";
    for (const auto& argument : arguments)
    {
        forget_owned_value(argument);
    }
    emit_stack_location();
    if (item.type == value_type::void_type)
    {
        write_instruction(invocation);
        if (!borrow_closure)
        {
            release(closure);
        }
        return {item.type, {}};
    }
    const auto result = temporary();
    write_instruction(result + " = " + invocation);
    if (!borrow_closure)
    {
        release(closure);
    }
    return own_direct_value({item.type, result});
}

llvm_code_generator::ir_value llvm_code_generator::emit_call(
    const expression& item, const call_expression& call)
{
    if (const auto local = emit_local_guard_call(item, call))
    {
        return *local;
    }
    if (!scalar_record_type(item.type))
    {
        if (const auto* target = native_record_target(call))
        {
            std::vector<const expression*> arguments;
            for (const auto& argument : call.arguments)
            {
                arguments.push_back(argument.value.get());
            }
            return native_record_call(*target, call.receiver.get(), arguments, item.position);
        }
    }
    if ((call.container_type && call.container_type->is_sum_type()) ||
        (call.receiver && call.receiver->type.is_sum_type()))
    {
        return emit_sum_call(item, call);
    }
    if (call.receiver && (call.receiver->type.is_iterator() ||
        (call.receiver->type.is_vector() &&
         (call.name == "snapshot_iter" || call.name == "live_iter"))))
    {
        return emit_iterator_call(item, call);
    }
    if ((call.container_type &&
         (call.container_type->is_typed_container() ||
          call.container_type->is_priority_entry())) ||
        (call.receiver && call.receiver->type.is_typed_container()))
    {
        return emit_container_call(item, call);
    }
    if (call.container_type || (call.receiver && call.receiver->type.is_vector()))
    {
        return emit_vector_call(item, call);
    }
    if (call.name == "len" && !call.receiver && !call.arguments.empty() &&
        call.arguments.front().value->type.is_typed_container())
    {
        const auto value = call_argument_value(call, 0);
        const auto result = container_operation(value.type, "size", {value},
                                                value_type::int_type, item.position);
        release(value);
        return result;
    }
    if (call.name == "len" && !call.receiver && !call.arguments.empty() &&
        call.arguments.front().value->type.is_vector())
    {
        bool borrowed = false;
        const auto value = container_value(*call.arguments.front().value, true, borrowed);
        const auto result = vector_length(value, false);
        if (!borrowed)
        {
            release(value);
        }
        return result;
    }
    if (call.is_super_view)
    {
        const auto value = load(find_variable("self", item.position));
        return {item.type, value.text};
    }
    if (call.receiver)
    {
        return emit_method_call(item, call);
    }
    if (call.indirect)
    {
        return emit_callback_call(item, call);
    }
    if (call.name == "move")
    {
        const auto& source = *call.arguments.front().value;
        if (item.ownership_id == 0)
        {
            return expression_value(source);
        }
        const auto* name = std::get_if<name_reference>(&source.data);
        if (name == nullptr)
        {
            throw compile_error(item.position,
                "LLVM 后端的 move 目标必须是简单变量名");
        }
        const auto variable = find_variable(name->name, source.position);
        materialize_native_parse(variable);
        const auto value = temporary();
        write_instruction(value + " = load ptr, ptr " + variable.address);
        write_instruction("store ptr null, ptr " + variable.address);
        return {item.type, value};
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
    if (call.name == "bind")
    {
        return emit_bind_call(item, call);
    }
    if (is_builtin_call(call.name))
    {
        if (call.name == "len" || call.name == "is_none")
        {
            bool borrowed = false;
            const auto& input = *call.arguments.front().value;
            if (call.name == "len")
            {
                if (auto length = emit_result_length(input))
                {
                    return *length;
                }
                if (auto length = emit_parse_error_field(input, true))
                {
                    return *length;
                }
                if (auto length = emit_record_string_length(input))
                {
                    return *length;
                }
            }
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
            if (call.name == "len" && input.type == value_type::dict_type)
            {
                if (const auto* name = std::get_if<name_reference>(&input.data))
                {
                    const auto slot = find_variable(name->name, input.position);
                    if (!slot.dict_reference.empty())
                    {
                        const auto reference = load_dict_reference(slot);
                        const auto length = temporary();
                        write_instruction(length +
                            " = call i64 @txrt_dict_ref_len(ptr " +
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
                (call.arguments.size() != init->parameters.size() ||
                 std::any_of(init->parameters.begin(), init->parameters.end(),
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
            parameters.push_back({field.name, field.type, field.position,
                                  parameter_kind::ordinary, {}});
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
    if (target.is_async)
    {
        return emit_async_call(item, call, target);
    }
    const bool needs_binding = call.arguments.size() != target.parameters.size() ||
        std::any_of(target.parameters.begin(),
        target.parameters.end(), [](const parameter& value)
        { return value.kind != parameter_kind::ordinary; }) ||
        std::any_of(call.arguments.begin(), call.arguments.end(),
            [](const call_argument& value)
            { return value.kind != argument_kind::positional; });
    if (target.external)
    {
        if (auto logged = emit_lazy_log(item, call, target))
        {
            return *logged;
        }
        if (auto formatted = emit_static_format(item, call, target))
        {
            return *formatted;
        }
        if (auto formatted = emit_dynamic_format(item, call, target))
        {
            return *formatted;
        }
        if (!needs_binding)
        {
            if (auto encoded = emit_constant_encoding(item, call, target))
            {
                return *encoded;
            }
            if (auto direct = emit_string_key_query(item, call, target))
            {
                return *direct;
            }
            if (auto direct = emit_literal_string_call(item, call, target))
            {
                return *direct;
            }
        }
        std::vector<ir_value> arguments;
        if (needs_binding)
        {
            arguments = emit_bound_arguments(item, call, target.parameters);
        }
        else
        {
            for (std::size_t index = 0; index < call.arguments.size(); ++index)
            {
                arguments.push_back(call_argument_value(call, index));
            }
        }
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
            const bool stable_suffix = std::all_of(
                call.arguments.begin() + index + 1, call.arguments.end(),
                simple_argument);
            const bool can_borrow = ordinary_parameter_borrowed(target, index) &&
                (argument.type != value_type::str_type || stable_suffix);
            arguments.push_back(can_borrow
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
