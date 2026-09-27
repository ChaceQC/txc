#include "frontend/ast/call_properties.hpp"

#include <algorithm>

namespace tx
{
namespace
{

bool scalar_type(const value_type& type)
{
    return type == value_type::int_type || type == value_type::float_type ||
        type == value_type::bool_type || type == value_type::str_type;
}

bool stable_index(const index_expression& index)
{
    const auto& type = index.object->type;
    // 对象键和有比较器的有序容器可能在索引期间回调用户代码。
    const bool known_lookup = type == value_type::array_type ||
        type == value_type::dict_type || type.is_vector() || type.is_deque() ||
        (type.container_name() == "map" && scalar_type(type.parameters.front()));
    return known_lookup && stable_borrow_expression(*index.object) &&
        stable_borrow_expression(*index.index);
}

} // namespace

bool scalar_container_type(const value_type& type)
{
    const auto kind = type.container_name();
    // heap 即使存基础类型也能持有用户提供的比较函数。
    return (kind == "map" || kind == "set" || kind == "queue" ||
            kind == "deque" || type.is_vector()) &&
        !type.parameters.empty() &&
        std::all_of(type.parameters.begin(), type.parameters.end(), scalar_type);
}

call_effects container_call_effects(
    const value_type& type, std::string_view operation)
{
    if ((type.is_typed_container() || type.is_vector()) &&
        (operation == "size" || operation == "empty" || operation == "capacity"))
    {
        return {false, false, false, false, false};
    }
    if (!scalar_container_type(type))
    {
        return {};
    }
    const bool query = operation == "size" || operation == "empty" ||
        operation == "capacity" || operation == "contains" ||
        operation == "read" || operation == "get" || operation == "front" ||
        operation == "back";
    const bool snapshot = operation == "keys" || operation == "values" ||
        operation == "to_vector" || operation == "entries" || operation == "to_array";
    const bool saves = operation == "set" || operation == "push" ||
        operation == "push_front" || operation == "push_back" ||
        operation == "insert" || operation == "resize" || operation == "assign" ||
        operation == "build" || operation == "new" || operation == "from_array";
    const bool mutation = saves || operation == "clear" || operation == "remove" ||
        operation == "erase" || operation == "pop" || operation == "pop_front" ||
        operation == "pop_back" || operation == "reserve";
    if (!query && !snapshot && !mutation)
    {
        return {};
    }
    // 返回文本只增加内部引用，不会调用用户析构。
    return {!query, false, false, !query && !snapshot, saves};
}

bool stable_borrow_expression(const expression& item)
{
    if (std::holds_alternative<name_reference>(item.data) ||
        std::holds_alternative<integer_literal>(item.data) ||
        std::holds_alternative<floating_literal>(item.data) ||
        std::holds_alternative<boolean_literal>(item.data) ||
        std::holds_alternative<string_literal>(item.data) ||
        std::holds_alternative<none_literal>(item.data))
    {
        return true;
    }
    if (const auto* member = std::get_if<member_expression>(&item.data))
    {
        return stable_borrow_expression(*member->object);
    }
    if (const auto* index = std::get_if<index_expression>(&item.data))
    {
        return stable_index(*index);
    }
    if (const auto* unary = std::get_if<unary_operation>(&item.data))
    {
        return !unary->binding && stable_borrow_expression(*unary->operand);
    }
    if (const auto* binary = std::get_if<binary_operation>(&item.data))
    {
        return !binary->binding && stable_borrow_expression(*binary->left) &&
            stable_borrow_expression(*binary->right);
    }
    if (const auto* call = std::get_if<call_expression>(&item.data))
    {
        const auto& effects = call->properties.effects;
        // 仅把返回基础值的只读调用用于证明后续实参稳定，避免临时对象析构。
        return scalar_type(item.type) && effects.allows_borrow() &&
            !effects.mutates_arguments && !effects.saves_arguments &&
            (!call->receiver || stable_borrow_expression(*call->receiver)) &&
            std::all_of(call->arguments.begin(), call->arguments.end(),
                [](const call_argument& argument)
                {
                    return stable_borrow_expression(*argument.value);
                });
    }
    return false;
}

} // namespace tx
