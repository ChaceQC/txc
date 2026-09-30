#include "backend/llvm/codegen.hpp"

#include <algorithm>

namespace tx
{

void llvm_code_generator::emit_function(const function_decl& function, bool native)
{
    if (function.external)
    {
        return;
    }
    const bool bounded = generating_bounded_integer_;
    proven_integer_function_ = bounded ? &function : nullptr;
    const auto symbol = function.owner_class.empty() ? function.name
        : class_method_symbol(function.owner_class, function.name);
    const auto& overloads = functions_.at(symbol);
    const auto found = std::find(overloads.begin(), overloads.end(), &function);
    const auto index = static_cast<std::size_t>(found - overloads.begin());
    native_result_type_ = native && scalar_record_type(function.return_type)
        ? function.return_type : value_type::void_type;
    return_type_ = native_result_type_ != value_type::void_type ? value_type::void_type : function.return_type;
    allocations_.str({});
    allocations_.clear();
    body_.str({});
    body_.clear();
    scopes_.clear();
    emitted_bounded_calls_.clear();
    error_roots_.clear();
    error_root_indices_.clear();
    pointer_owners_.clear();
    error_targets_.clear();
    next_value_ = 0;
    next_slot_ = 0;
    next_label_ = 0;
    current_method_owner_ = function.owner_class;
    current_function_body_ = &function.body;
    current_analysis_ = program_analysis_.find(function);
    current_statement_position_ = &function.position;
    last_stack_position_.reset();
    entry_scalar_field_cache_.clear();
    terminated_ = false;
    in_entry_block_ = true;
    push_scope();

    std::string parameters = "ptr %tx_context";
    if (native_result_type_ != value_type::void_type)
    {
        parameters += ", ptr %tx_result";
    }
    const bool fixed_method = !native && static_record_type(value_type(function.owner_class));
    if (native && !function.owner_class.empty())
    {
        parameters += ", ptr %arg0";
        variable_slot self{value_type(function.owner_class), "%arg0", true};
        self.stack_record = "%arg0";
        scopes_.back().emplace("self", std::move(self));
    }
    else if (!function.owner_class.empty())
    {
        parameters += ", ptr %arg0";
        const auto address = allocate(value_type(function.owner_class),
                                      function.position, false);
        write_instruction("store ptr %arg0, ptr " + address);
        variable_slot self{value_type(function.owner_class), address, true};
        if (fixed_method)
        {
            parameters += ", ptr %tx_self_view";
            self.record_view = allocate(value_type::any_type, function.position, false);
            write_instruction("store ptr %tx_self_view, ptr " + self.record_view);
        }
        else
        {
            cache_record_view(self, "%arg0");
        }
        scopes_.back().emplace("self", std::move(self));
    }
    for (std::size_t i = 0; i < function.parameters.size(); ++i)
    {
        const auto& parameter = function.parameters[i];
        parameters += ", ";
        const auto abi_type = parameter_abi_type(parameter);
        const auto type = llvm_type(abi_type, parameter.position);
        const auto argument_index = i + (function.owner_class.empty() ? 0 : 1);
        parameters += type + " %arg" + std::to_string(argument_index);
        if (native && scalar_record_type(abi_type))
        {
            variable_slot slot{abi_type, "%arg" + std::to_string(argument_index), true};
            slot.stack_record = slot.address;
            scopes_.back().emplace(parameter.name, std::move(slot));
            continue;
        }
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
        if (bounded)
        {
            slot.integer_range = integer_interval{0, 256};
        }
        cache_vector_reference(slot, "%arg" + std::to_string(argument_index));
        cache_record_view(slot, "%arg" + std::to_string(argument_index));
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
        emit_stack_pop();
        write_instruction(function.return_type == value_type::void_type
            ? "ret void" : "unreachable");
        terminated_ = true;
    }
    pop_scope();

    module_ << "define " << llvm_type(return_type_, function.position)
            << ' ' << function_name(symbol, index) << (bounded ? "_bounded" : native ? "_native" : fixed_method ? "_record" : "")
            << '(' << parameters
            << ") {\nentry:\n";
    write_stack_frame(function);
    module_ << allocations_.str() << body_.str() << "}\n\n";
    if (fixed_method)
    {
        emit_record_method_adapter(function, symbol, index);
    }
    if (!bounded && !native && function.owner_class.empty() &&
        std::all_of(function.parameters.begin(), function.parameters.end(),
            [](const parameter& item)
            { return item.kind == parameter_kind::ordinary; }))
    {
        emit_callback_wrapper(function, symbol, index);
    }
    current_method_owner_.clear();
    current_function_body_ = nullptr;
    current_analysis_ = nullptr;
    current_statement_position_ = nullptr;
    native_result_type_ = value_type::void_type;
    proven_integer_function_ = nullptr;
    if (!bounded && !native && native_record_function(function))
    {
        emit_function(function, true);
    }
    if (!bounded && !native && bounded_integer_result(function))
    {
        generating_bounded_integer_ = true;
        emit_function(function);
        generating_bounded_integer_ = false;
    }
}

} // namespace tx
