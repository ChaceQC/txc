#include "backend/llvm/codegen.hpp"

#include <algorithm>

namespace tx
{

void llvm_code_generator::collect_class_nodes(
    const class_decl& definition, std::vector<const class_decl*>& result) const
{
    if (std::find(result.begin(), result.end(), &definition) != result.end())
    {
        return;
    }
    for (const auto& base : definition.bases)
    {
        collect_class_nodes(*classes_.at(base), result);
    }
    result.push_back(&definition);
}

std::size_t llvm_code_generator::class_field_count(
    const class_decl& definition) const
{
    std::vector<const class_decl*> nodes;
    collect_class_nodes(definition, nodes);
    std::size_t count = 0;
    for (const auto* node : nodes)
    {
        for (const auto& field : node->fields)
        {
            count = std::max(count, field.slot + 1);
        }
    }
    return count;
}

bool llvm_code_generator::class_is_assignable(
    const value_type& actual, const value_type& expected) const
{
    if (actual == expected)
    {
        return true;
    }
    const auto found = classes_.find(actual.name);
    if (found == classes_.end())
    {
        return false;
    }
    for (const auto& base : found->second->bases)
    {
        if (class_is_assignable(value_type(base), expected))
        {
            return true;
        }
    }
    return false;
}

void llvm_code_generator::emit_class_metadata(const class_decl& definition)
{
    const auto write_ptrs = [this](const std::string& symbol,
                                   const std::vector<std::string>& values)
    {
        if (values.empty())
        {
            return;
        }
        globals_ << symbol << " = private constant [" << values.size()
                 << " x ptr] [";
        for (std::size_t index = 0; index < values.size(); ++index)
        {
            if (index != 0)
            {
                globals_ << ", ";
            }
            globals_ << "ptr " << values[index];
        }
        globals_ << "]\n";
    };
    std::vector<const class_decl*> nodes;
    collect_class_nodes(definition, nodes);
    std::vector<std::string> ancestors;
    for (const auto* node : nodes)
    {
        ancestors.push_back(global_bytes(node->name));
    }
    write_ptrs("@tx_class_ancestors_" + definition.name, ancestors);
    std::vector<std::string> field_types(class_field_count(definition), "null");
    for (const auto* node : nodes)
    {
        for (const auto& field : node->fields)
        {
            field_types[field.slot] = global_bytes(field.type.name);
        }
    }
    write_ptrs("@tx_class_fields_" + definition.name, field_types);
    std::vector<std::string> targets(virtual_slot_count_, "null");
    for (std::size_t slot = 0; slot < definition.virtual_targets.size(); ++slot)
    {
        const auto& target = definition.virtual_targets[slot];
        if (!target.symbol.empty())
        {
            targets[slot] = function_name(target.symbol, target.overload_index);
        }
    }
    write_ptrs("@tx_class_vtable_" + definition.name, targets);
    std::vector<std::string> destructors;
    for (auto node = nodes.rbegin(); node != nodes.rend(); ++node)
    {
        for (const auto& method : (*node)->methods)
        {
            if (method.name == "deinit")
            {
                destructors.push_back(function_name(
                    class_method_symbol((*node)->name, "deinit"),
                    method.overload_index));
            }
        }
    }
    write_ptrs("@tx_class_destructors_" + definition.name, destructors);
}

llvm_code_generator::ir_value llvm_code_generator::emit_class_constructor(
    const expression& item, const call_expression& call,
    const std::vector<ir_value>& arguments,
    const std::vector<bool>& borrowed_arguments)
{
    const auto& definition = *classes_.at(call.name);
    std::vector<const class_decl*> nodes;
    collect_class_nodes(definition, nodes);
    std::size_t destructor_count = 0;
    for (const auto* node : nodes)
    {
        destructor_count += static_cast<std::size_t>(std::any_of(
            node->methods.begin(), node->methods.end(),
            [](const function_decl& method) { return method.name == "deinit"; }));
    }
    const auto separator = call.source_name.find_last_of('.');
    const auto display_name = call.source_name.substr(
        separator == std::string::npos ? 0 : separator + 1);
    const auto field_count = class_field_count(definition);
    const auto address = allocate(item.type, item.position);
    const auto status = temporary();
    if (static_record_type(item.type))
    {
        write_instruction(status + " = call i32 @txrt_record_class_new(ptr @tx_record_" +
                          item.type.name + ", ptr " + address + ")");
    }
    else
    {
        write_instruction(status + " = call i32 @txrt_class_new(ptr " +
            global_bytes(call.name) + ", ptr " + global_bytes(display_name) +
            ", ptr @tx_class_ancestors_" + call.name + ", i64 " +
            std::to_string(nodes.size()) + ", ptr " +
            (field_count == 0 ? "null" : "@tx_class_fields_" + call.name) +
            ", i64 " + std::to_string(field_count) + ", ptr " +
            (virtual_slot_count_ == 0 ? "null" : "@tx_class_vtable_" + call.name) +
            ", i64 " + std::to_string(virtual_slot_count_) + ", ptr " +
            (destructor_count == 0 ? "null" :
                "@tx_class_destructors_" + call.name) +
            ", i64 " + std::to_string(destructor_count) +
            ", ptr " + address + ")");
    }
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    const auto object = temporary();
    write_instruction(object + " = load ptr, ptr " + address);
    initialize_container_fields(definition, object, item.position);
    if (!call.constructor_init_symbol.empty())
    {
        const auto& init = *functions_.at(call.constructor_init_symbol).at(
            call.constructor_init_index);
        auto owned_arguments = arguments;
        owned_arguments.insert(owned_arguments.begin(), {item.type, object});
        const auto passed = coerce_nullable_arguments(init, owned_arguments);
        std::string parameters = "ptr %tx_context, ptr " + object;
        for (std::size_t index = 1; index < passed.size(); ++index)
        {
            const auto& argument = passed[index];
            parameters += ", " + llvm_type(argument.type, item.position) +
                          " " + argument.text;
        }
        if (init.external_name == "requests.session.init")
        {
            emit_stack_location();
            write_instruction("call void " + function_name(
                "m0_bridge_session_init", call.constructor_init_index) +
                "(" + parameters + ")");
            if (!recoverable_errors_)
            {
                const auto pending = temporary();
                write_instruction(pending + " = load i32, ptr %tx_error_kind");
                write_instruction("call void @txrt_require_success(i32 " +
                                  pending + ")");
            }
        }
        else
        {
            transfer_call_arguments(init, passed);
            emit_stack_location();
            write_instruction("call void " + function_name(
                call.constructor_init_symbol, call.constructor_init_index) +
                "(" + parameters + ")");
            for (std::size_t index = 0; index < arguments.size(); ++index)
            {
                if (init_parameter_borrowed(init, index) &&
                    !borrowed_arguments[index])
                {
                    release(arguments[index]);
                }
            }
        }
    }
    return {item.type, object};
}

llvm_code_generator::ir_value llvm_code_generator::emit_method_call(
    const expression& item, const call_expression& call)
{
    bool borrowed = false;
    const auto receiver = expression_value_or_borrow(*call.receiver, borrowed);
    const auto& target = *functions_.at(call.name).at(*call.overload_index);
    std::string receiver_view;
    if (!call.virtual_dispatch && !target.external &&
        static_record_type(value_type(target.owner_class)))
    {
        const auto* name = std::get_if<name_reference>(&call.receiver->data);
        const auto cached = name ? find_variable(name->name, item.position).record_view : std::string{};
        if (!cached.empty())
        {
            receiver_view = temporary();
            write_instruction(receiver_view + " = load ptr, ptr " + cached);
        }
        else
        {
            receiver_view = record_view_value(receiver);
        }
    }
    const bool needs_binding = call.arguments.size() != target.parameters.size() ||
        std::any_of(target.parameters.begin(),
        target.parameters.end(), [](const parameter& value)
        { return value.kind != parameter_kind::ordinary; }) ||
        std::any_of(call.arguments.begin(), call.arguments.end(),
            [](const call_argument& value)
            { return value.kind != argument_kind::positional; });
    std::vector<ir_value> arguments = needs_binding
        ? emit_bound_arguments(item, call, target.parameters)
        : std::vector<ir_value>{};
    if (!needs_binding)
    {
        for (const auto& argument : call.arguments)
        {
            arguments.push_back(expression_value(*argument.value));
        }
    }
    arguments.insert(arguments.begin(), receiver);
    if (target.external_name.starts_with("requests.session.") ||
        target.external_name.starts_with("requests.response."))
    {
        return emit_precompiled_requests_method(item, call, target, arguments,
                                                borrowed);
    }
    if (!call.virtual_dispatch)
    {
        const auto result = emit_user_call(item, call, target, arguments, {}, receiver_view);
        if (!borrowed)
        {
            release(receiver);
        }
        return result;
    }
    const auto target_address = temporary();
    if (static_record_type(receiver.type))
    {
        const auto view = record_view_value(receiver);
        const auto type_slot = temporary();
        const auto type = temporary();
        const auto table_slot = temporary();
        const auto table = temporary();
        const auto target_slot = temporary();
        write_instruction(type_slot + " = getelementptr inbounds %tx_record_view, ptr " + view + ", i32 0, i32 1");
        write_instruction(type + " = load ptr, ptr " + type_slot);
        write_instruction(table_slot + " = getelementptr inbounds %tx_record_type, ptr " + type + ", i32 0, i32 7");
        write_instruction(table + " = load ptr, ptr " + table_slot);
        write_instruction(target_slot + " = getelementptr inbounds ptr, ptr " + table +
                          ", i64 " + std::to_string(call.virtual_slot));
        write_instruction(target_address + " = load ptr, ptr " + target_slot);
    }
    else
    {
        write_instruction(target_address +
        " = call ptr @txrt_class_virtual_target_fast(ptr " +
        receiver.text + ", i64 " + std::to_string(call.virtual_slot) + ")");
    }
    const auto passed = coerce_nullable_arguments(target, arguments);
    std::string parameters = "ptr %tx_context";
    for (const auto& argument : passed)
    {
        if (!parameters.empty())
        {
            parameters += ", ";
        }
        parameters += llvm_type(argument.type, item.position) + " " + argument.text;
    }
    const auto invocation = "call " + llvm_type(item.type, item.position) +
        " " + target_address + "(" + parameters + ")";
    transfer_call_arguments(target, passed);
    emit_stack_location();
    if (item.type == value_type::void_type)
    {
        write_instruction(invocation);
        if (!borrowed)
        {
            release(receiver);
        }
        return {item.type, {}};
    }
    const auto result = temporary();
    write_instruction(result + " = " + invocation);
    const auto owned_result = own_direct_value({item.type, result});
    if (!borrowed)
    {
        release(receiver);
    }
    return owned_result;
}

llvm_code_generator::ir_value
llvm_code_generator::emit_precompiled_requests_method(
    const expression& item, const call_expression& call,
    const function_decl& target, const std::vector<ir_value>& arguments,
    bool borrowed_receiver)
{
    const auto passed = coerce_nullable_arguments(target, arguments);
    const bool session_method = target.external_name.starts_with(
        "requests.session.");
    const auto symbol = session_method
        ? "m0_bridge_session_" + target.external_name.substr(17)
        : "m0_bridge_" + target.external_name.substr(18);
    std::string parameters = "ptr %tx_context";
    for (const auto& argument : passed)
    {
        if (!parameters.empty())
        {
            parameters += ", ";
        }
        parameters += llvm_type(argument.type, item.position) + " " +
            argument.text;
    }
    const auto return_type = llvm_type(item.type, item.position);
    const auto invocation = "call " + return_type + " " +
        function_name(symbol, *call.overload_index) + "(" + parameters + ")";
    emit_stack_location();
    for (std::size_t index = 1; index < passed.size(); ++index)
    {
        if (target.parameters[index - 1].kind != parameter_kind::ordinary ||
            parameter_is_nullable(target.parameters[index - 1]))
        {
            forget_owned_value(passed[index]);
        }
    }
    ir_value result{item.type, {}};
    if (item.type == value_type::void_type)
    {
        write_instruction(invocation);
    }
    else
    {
        result.text = temporary();
        write_instruction(result.text + " = " + invocation);
    }
    if (!recoverable_errors_)
    {
        const auto status = temporary();
        write_instruction(status + " = load i32, ptr %tx_error_kind");
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
    }
    for (std::size_t index = 1; index < passed.size(); ++index)
    {
        if (target.parameters[index - 1].kind == parameter_kind::ordinary &&
            !parameter_is_nullable(target.parameters[index - 1]))
        {
            release(passed[index]);
        }
    }
    if (!borrowed_receiver)
    {
        release(arguments.front());
    }
    return own_direct_value(result);
}

} // namespace tx
