#include "backend/llvm/codegen.hpp"
#include "common/local_resource_layout.hpp"

#include <algorithm>

namespace tx
{

bool llvm_code_generator::eligible_local_class(const variable_declaration& declaration) const
{
    const auto* call = declaration.initializer
        ? std::get_if<call_expression>(&declaration.initializer->data) : nullptr;
    if (!call || !call->is_constructor || !classes_.contains(call->name) ||
        !static_record_type(declaration.initializer->type) || !field_only_local(declaration))
    {
        return false;
    }
    const auto& definition = *classes_.at(call->name);
    std::vector<const class_decl*> nodes;
    collect_class_nodes(definition, nodes);
    std::size_t destructors = 0;
    for (const auto* node : nodes)
    {
        for (const auto& field : node->fields)
        {
            // 其他类型需要调用容器初始化器，仍由正常构造路径负责。
            if (field.type.is_typed_container() || field.type.is_sum_type() || field.type.is_iterator())
            {
                return false;
            }
        }
        for (const auto& method : node->methods)
        {
            if (method.name == "deinit" && (++destructors > 1 || !field_only_self(method)))
            {
                return false;
            }
        }
    }
    const function_decl* init = call->constructor_init_symbol.empty() ? nullptr :
        functions_.at(call->constructor_init_symbol).at(call->constructor_init_index);
    if ((init && (!field_only_self(*init) || init->parameters.size() != call->arguments.size() ||
        std::any_of(init->parameters.begin(), init->parameters.end(), [](const parameter& parameter)
        {
            return parameter.kind != parameter_kind::ordinary || parameter_is_nullable(parameter);
        }))) || std::any_of(call->arguments.begin(), call->arguments.end(), [](const call_argument& argument)
        {
            return argument.kind != argument_kind::positional;
        }))
    {
        return false;
    }
    return true;
}

bool llvm_code_generator::emit_local_class(const statement& item, const variable_declaration& declaration)
{
    if (!eligible_local_class(declaration))
    {
        return false;
    }
    const auto* call = std::get_if<call_expression>(&declaration.initializer->data);
    const function_decl* init = call->constructor_init_symbol.empty() ? nullptr :
        functions_.at(call->constructor_init_symbol).at(call->constructor_init_index);
    std::vector<ir_value> arguments{{declaration.initializer->type, "null", true}};
    for (const auto& argument : call->arguments)
    {
        arguments.push_back(expression_value(*argument.value));
    }
    const auto storage = "%slot" + std::to_string(next_slot_++);
    const auto cleanup = emit_local_class_cleanup(*classes_.at(call->name));
    const auto layout = "[" + std::to_string(local_class_bytes) + " x i8]";
    allocations_ << "  " << storage << " = alloca " << layout << ", align " << local_class_alignment
                 << "\n  store " << layout << " zeroinitializer, ptr " << storage << '\n';
    if (recoverable_errors_)
    {
        error_roots_.push_back({declaration.initializer->type, storage, scopes_.size(), cleanup});
    }
    const auto view_slot = allocate(value_type::any_type, item.position, false);
    const auto status = temporary();
    write_instruction(status + " = call i32 @txrt_class_local_new(ptr @tx_record_" + call->name +
        ", ptr " + storage + ", ptr " + view_slot + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    const auto view = temporary();
    write_instruction(view + " = load ptr, ptr " + view_slot);
    if (init)
    {
        std::string parameters = "ptr %tx_context, ptr null, ptr " + view;
        for (std::size_t index = 1; index < arguments.size(); ++index)
        {
            parameters += ", " + llvm_type(arguments[index].type, item.position) + " " + arguments[index].text;
        }
        transfer_call_arguments(*init, arguments);
        emit_stack_location();
        write_instruction("call void " + function_name(call->constructor_init_symbol, call->constructor_init_index) +
            "_record(" + parameters + ")");
        for (std::size_t index = 1; index < arguments.size(); ++index)
        {
            if (init_parameter_borrowed(*init, index - 1))
            {
                release(arguments[index]);
            }
        }
    }
    const auto handle = allocate(value_type::any_type, item.position, false);
    write_instruction("store ptr null, ptr " + handle);
    variable_slot slot{declaration.initializer->type, handle, true};
    slot.record_view = view_slot;
    slot.local_class_storage = storage;
    slot.local_class_cleanup = cleanup;
    scopes_.back().emplace(declaration.name, std::move(slot));
    return true;
}

std::string llvm_code_generator::emit_local_class_cleanup(const class_decl& definition)
{
    const auto symbol = "tx_class_local_cleanup_" + std::to_string(next_string_++);
    std::vector<const class_decl*> nodes;
    collect_class_nodes(definition, nodes);
    std::string destructor = "null";
    for (const auto* node : nodes)
    {
        for (const auto& method : node->methods)
        {
            if (method.name == "deinit")
            {
                destructor = "@" + symbol + "_deinit";
                module_ << "define internal void " << destructor
                    << "(ptr %context, ptr %view) uwtable {\nentry:\n  call void "
                    << function_name(class_method_symbol(node->name, "deinit"), method.overload_index)
                    << "_record(ptr %context, ptr null, ptr %view)\n  ret void\n}\n\n";
            }
        }
    }
    module_ << "define internal void @" << symbol << "(ptr %storage) uwtable {\nentry:\n"
        << "  call void @txrt_class_local_destroy(ptr %storage, ptr " << destructor << ")\n"
        << "  ret void\n}\n\n";
    return symbol;
}

} // namespace tx
