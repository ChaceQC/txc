#include "backend/llvm/codegen.hpp"

#include <algorithm>

namespace tx
{

void llvm_code_generator::emit_function(const function_decl& function)
{
    if (function.external)
    {
        return;
    }
    const auto symbol = function.owner_class.empty() ? function.name
        : class_method_symbol(function.owner_class, function.name);
    const auto& overloads = functions_.at(symbol);
    const auto found = std::find(overloads.begin(), overloads.end(), &function);
    const auto index = static_cast<std::size_t>(found - overloads.begin());
    return_type_ = function.return_type;
    allocations_.str({});
    allocations_.clear();
    body_.str({});
    body_.clear();
    scopes_.clear();
    error_roots_.clear();
    error_root_indices_.clear();
    pointer_owners_.clear();
    error_targets_.clear();
    next_value_ = 0;
    next_slot_ = 0;
    next_label_ = 0;
    random_context_slot_.clear();
    current_method_owner_ = function.owner_class;
    current_function_body_ = &function.body;
    entry_scalar_field_cache_.clear();
    terminated_ = false;
    in_entry_block_ = true;
    push_scope();

    std::string parameters;
    if (!function.owner_class.empty())
    {
        parameters = "ptr %arg0";
        const auto address = allocate(value_type(function.owner_class),
                                      function.position, false);
        write_instruction("store ptr %arg0, ptr " + address);
        scopes_.back().emplace("self",
            variable_slot{value_type(function.owner_class), address, true});
    }
    for (std::size_t i = 0; i < function.parameters.size(); ++i)
    {
        const auto& parameter = function.parameters[i];
        if (i != 0 || !function.owner_class.empty())
        {
            parameters += ", ";
        }
        const auto abi_type = parameter_abi_type(parameter);
        const auto type = llvm_type(abi_type, parameter.position);
        const auto argument_index = i + (function.owner_class.empty() ? 0 : 1);
        parameters += type + " %arg" + std::to_string(argument_index);
        const bool borrowed = (i == 0 && operator_parameter_borrowed(function)) ||
            init_parameter_borrowed(function, i) || ordinary_parameter_borrowed(function, i);
        const auto address = allocate(abi_type, parameter.position, !borrowed);
        write_instruction("store " + type + " %arg" +
                          std::to_string(argument_index) +
                          ", ptr " + address);
        const auto array_reference = abi_type == value_type::array_type
            ? cache_array_reference("%arg" + std::to_string(argument_index),
                                    parameter.position) : std::string{};
        variable_slot slot{abi_type, address,
            (i == 0 && operator_parameter_borrowed(function)) ||
            init_parameter_borrowed(function, i) ||
            ordinary_parameter_borrowed(function, i), array_reference};
        if (abi_type == value_type::dict_type)
        {
            slot.dict_reference = cache_dict_reference(
                "%arg" + std::to_string(argument_index), parameter.position);
        }
        scopes_.back().emplace(parameter.name, std::move(slot));
    }
    emit_statements(function.body);
    if (!terminated_)
    {
        for (const auto& [name, variable] : scopes_.back())
        {
            (void)name;
            release_slot(variable);
        }
        body_ << "  call void @txrt_stack_pop()\n";
        write_instruction(function.return_type == value_type::void_type
            ? "ret void" : "unreachable");
        terminated_ = true;
    }
    pop_scope();

    const auto display_name = function.source_name.empty()
        ? function.name : function.source_name;
    module_ << "define " << llvm_type(function.return_type, function.position)
            << ' ' << function_name(symbol, index) << '(' << parameters
            << ") {\nentry:\n" << allocations_.str()
            << "  call void @txrt_stack_push(ptr " << global_bytes(display_name)
            << ", ptr " << global_bytes(function.position.file) << ", i64 "
            << function.position.line << ", i64 " << function.position.column
            << ")\n" << body_.str() << "}\n\n";
    if (function.owner_class.empty() &&
        std::all_of(function.parameters.begin(), function.parameters.end(),
            [](const parameter& item)
            { return item.kind == parameter_kind::ordinary; }))
    {
        emit_callback_wrapper(function, symbol, index);
    }
    current_method_owner_.clear();
    current_function_body_ = nullptr;
}

} // namespace tx
