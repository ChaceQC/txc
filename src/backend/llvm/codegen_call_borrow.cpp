#include "backend/llvm/codegen.hpp"

#include <algorithm>

namespace tx
{

llvm_code_generator::ir_value llvm_code_generator::call_argument_value(
    const call_expression& call, std::size_t index)
{
    const auto& argument = *call.arguments[index].value;
    const bool stable_suffix = std::all_of(
        call.arguments.begin() + index + 1, call.arguments.end(),
        [](const call_argument& following)
        {
            return stable_value_expression(*following.value);
        });
    const bool can_borrow = index < call.properties.arguments.size() &&
        call.properties.arguments[index] == argument_ownership::borrowed &&
        call.properties.effects.allows_borrow() && stable_suffix;
    bool borrowed = false;
    auto value = can_borrow && is_value_handle(argument.type)
        ? container_value(argument, true, borrowed)
        : can_borrow && argument.type == value_type::str_type
        ? read_only_string_value(argument, borrowed)
        : can_borrow ? expression_value_or_borrow(argument, borrowed)
                     : expression_value(argument);
    value.borrowed = borrowed;
    return value;
}

} // namespace tx
