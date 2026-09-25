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
    std::vector<std::string> field_types(field_slot_count_, "null");
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
    const std::vector<ir_value>& arguments)
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
    const auto address = allocate(item.type, item.position);
    const auto status = temporary();
    write_instruction(status + " = call i32 @txrt_class_new(ptr " +
        global_bytes(call.name) + ", ptr " + global_bytes(display_name) +
        ", ptr @tx_class_ancestors_" + call.name + ", i64 " +
        std::to_string(nodes.size()) + ", ptr " +
        (field_slot_count_ == 0 ? "null" : "@tx_class_fields_" + call.name) +
        ", i64 " + std::to_string(field_slot_count_) + ", ptr " +
        (virtual_slot_count_ == 0 ? "null" : "@tx_class_vtable_" + call.name) +
        ", i64 " + std::to_string(virtual_slot_count_) + ", ptr " +
        (destructor_count == 0 ? "null" :
            "@tx_class_destructors_" + call.name) +
        ", i64 " + std::to_string(destructor_count) +
        ", ptr " + address + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    const auto object = temporary();
    write_instruction(object + " = load ptr, ptr " + address);
    if (!call.constructor_init_symbol.empty())
    {
        const auto clone_slot = allocate(item.type, item.position);
        const auto clone_status = temporary();
        write_instruction(clone_status + " = call i32 @txrt_value_clone(ptr " +
                          object + ", ptr " + clone_slot + ")");
        write_instruction("call void @txrt_require_success(i32 " +
                          clone_status + ")");
        const auto receiver = temporary();
        write_instruction(receiver + " = load ptr, ptr " + clone_slot);
        std::string parameters = "ptr " + receiver;
        for (const auto& argument : arguments)
        {
            parameters += ", " + llvm_type(argument.type, item.position) +
                          " " + argument.text;
        }
        write_instruction("call void " + function_name(
            call.constructor_init_symbol, call.constructor_init_index) +
            "(" + parameters + ")");
    }
    return {item.type, object};
}

llvm_code_generator::ir_value llvm_code_generator::emit_method_call(
    const expression& item, const call_expression& call)
{
    const auto receiver = expression_value(*call.receiver);
    const auto& target = *functions_.at(call.name).at(*call.overload_index);
    const bool needs_binding = std::any_of(target.parameters.begin(),
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
    if (!call.virtual_dispatch)
    {
        return emit_user_call(item, call, target, arguments);
    }
    const auto target_slot = allocate(value_type::any_type, item.position);
    const auto status = temporary();
    write_instruction(status + " = call i32 @txrt_class_virtual_target(ptr " +
        receiver.text + ", i64 " + std::to_string(call.virtual_slot) +
        ", ptr " + target_slot + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    const auto target_address = temporary();
    write_instruction(target_address + " = load ptr, ptr " + target_slot);
    std::string parameters;
    for (const auto& argument : arguments)
    {
        if (!parameters.empty())
        {
            parameters += ", ";
        }
        parameters += llvm_type(argument.type, item.position) + " " + argument.text;
    }
    const auto invocation = "call " + llvm_type(item.type, item.position) +
        " " + target_address + "(" + parameters + ")";
    if (item.type == value_type::void_type)
    {
        write_instruction(invocation);
        return {item.type, {}};
    }
    const auto result = temporary();
    write_instruction(result + " = " + invocation);
    return {item.type, result};
}

} // namespace tx
