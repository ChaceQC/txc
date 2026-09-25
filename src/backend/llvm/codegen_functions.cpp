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
    next_value_ = 0;
    next_slot_ = 0;
    next_label_ = 0;
    random_context_slot_.clear();
    current_method_owner_ = function.owner_class;
    terminated_ = false;
    push_scope();

    std::string parameters;
    if (!function.owner_class.empty())
    {
        parameters = "ptr %arg0";
        const auto address = allocate(value_type(function.owner_class),
                                      function.position);
        write_instruction("store ptr %arg0, ptr " + address);
        scopes_.back().emplace("self",
            variable_slot{value_type(function.owner_class), address});
    }
    for (std::size_t i = 0; i < function.parameters.size(); ++i)
    {
        const auto& parameter = function.parameters[i];
        if (i != 0 || !function.owner_class.empty())
        {
            parameters += ", ";
        }
        const auto type = llvm_type(parameter.type, parameter.position);
        const auto argument_index = i + (function.owner_class.empty() ? 0 : 1);
        parameters += type + " %arg" + std::to_string(argument_index);
        const auto address = allocate(parameter.type, parameter.position);
        write_instruction("store " + type + " %arg" +
                          std::to_string(argument_index) +
                          ", ptr " + address);
        scopes_.back().emplace(parameter.name,
                               variable_slot{parameter.type, address});
    }
    emit_statements(function.body);
    if (!terminated_)
    {
        for (const auto& [name, variable] : scopes_.back())
        {
            (void)name;
            release_slot(variable);
        }
        write_instruction(function.return_type == value_type::void_type
            ? "ret void" : "unreachable");
        terminated_ = true;
    }
    pop_scope();

    module_ << "define " << llvm_type(function.return_type, function.position)
            << ' ' << function_name(symbol, index) << '(' << parameters
            << ") {\nentry:\n" << allocations_.str() << body_.str() << "}\n\n";
    current_method_owner_.clear();
}

} // namespace tx
