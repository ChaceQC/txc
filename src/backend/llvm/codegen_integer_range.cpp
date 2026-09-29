#include "backend/llvm/codegen.hpp"

#include <algorithm>
#include <charconv>
#include <limits>

namespace tx
{
namespace
{

constexpr auto minimum = std::numeric_limits<std::int64_t>::min();
constexpr auto maximum = std::numeric_limits<std::int64_t>::max();

std::optional<std::int64_t> bound_operation(
    const std::string& name, std::int64_t left, std::int64_t right)
{
    if (name == "txrt_add_i64")
    {
        if ((right > 0 && left > maximum - right) ||
            (right < 0 && left < minimum - right))
        {
            return std::nullopt;
        }
        return left + right;
    }
    if (name == "txrt_sub_i64")
    {
        if ((right < 0 && left > maximum + right) ||
            (right > 0 && left < minimum + right))
        {
            return std::nullopt;
        }
        return left - right;
    }
    if ((left > 0 && ((right > 0 && left > maximum / right) ||
                     (right < 0 && right < minimum / left))) ||
        (left < 0 && ((right > 0 && left < minimum / right) ||
                     (right < 0 && left < maximum / right))))
    {
        return std::nullopt;
    }
    return left * right;
}

} // namespace

llvm_code_generator::integer_interval llvm_code_generator::integer_range_of(
    const ir_value& value)
{
    if (value.integer_range)
    {
        return *value.integer_range;
    }
    std::int64_t literal = 0;
    const auto* end = value.text.data() + value.text.size();
    const auto [parsed, error] = std::from_chars(value.text.data(), end, literal);
    if (error == std::errc{} && parsed == end)
    {
        return {literal, literal};
    }
    return {minimum, maximum};
}

std::optional<llvm_code_generator::ir_value> llvm_code_generator::emit_proven_arithmetic(
    const std::string& name, const ir_value& left, const ir_value& right)
{
    const auto* instruction = name == "txrt_add_i64" ? "add" :
        name == "txrt_sub_i64" ? "sub" : name == "txrt_mul_i64" ? "mul" : nullptr;
    if (!instruction)
    {
        return std::nullopt;
    }
    const auto a = integer_range_of(left);
    const auto b = integer_range_of(right);
    integer_interval bounds{maximum, minimum};
    for (const auto x : {a.first, a.second})
    {
        for (const auto y : {b.first, b.second})
        {
            const auto value = bound_operation(name, x, y);
            if (!value)
            {
                return std::nullopt;
            }
            bounds.first = std::min(bounds.first, *value);
            bounds.second = std::max(bounds.second, *value);
        }
    }
    ir_value result{value_type::int_type, temporary()};
    result.integer_range = bounds;
    write_instruction(result.text + " = " + instruction + " nsw i64 " +
        left.text + ", " + right.text);
    return result;
}

void llvm_code_generator::refine_integer_condition(const expression& condition,
    const std::vector<stmt_ptr>& body, bool truth)
{
    const auto* binary = std::get_if<binary_operation>(&condition.data);
    if (!binary || binary->binding || binary->left->type != value_type::int_type)
    {
        return;
    }
    const auto* name = std::get_if<name_reference>(&binary->left->data);
    const auto* literal = std::get_if<integer_literal>(&binary->right->data);
    if (!name || !literal || !scalar_local_unchanged(name->name, body))
    {
        return;
    }
    const auto constant = integer_range_of({value_type::int_type, literal->digits});
    if (constant.first != constant.second)
    {
        return;
    }
    auto slot = find_variable(name->name, condition.position);
    auto range = slot.integer_range.value_or(integer_interval{minimum, maximum});
    const auto value = constant.first;
    const auto kind = binary->operation;
    if ((truth && kind == token_kind::greater) || (!truth && kind == token_kind::less_equal))
    {
        if (value == maximum)
        {
            return;
        }
        range.first = std::max(range.first, value + 1);
    }
    else if ((truth && kind == token_kind::less) || (!truth && kind == token_kind::greater_equal))
    {
        if (value == minimum)
        {
            return;
        }
        range.second = std::min(range.second, value - 1);
    }
    else if ((truth && kind == token_kind::greater_equal) || (!truth && kind == token_kind::less))
    {
        range.first = std::max(range.first, value);
    }
    else if ((truth && kind == token_kind::less_equal) || (!truth && kind == token_kind::greater))
    {
        range.second = std::min(range.second, value);
    }
    else
    {
        return;
    }
    if (range.first <= range.second)
    {
        slot.integer_range = range;
        scopes_.back().emplace(name->name, std::move(slot));
    }
}

} // namespace tx
