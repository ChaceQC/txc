#include "backend/llvm/codegen.hpp"

#include <array>
#include <algorithm>
#include <charconv>
#include <functional>
#include <limits>

namespace tx
{
namespace
{
using interval = std::pair<std::int64_t, std::int64_t>;
constexpr std::int64_t bound = 256;

std::optional<std::int64_t> literal_value(const expression& value)
{
    if (const auto* literal = std::get_if<integer_literal>(&value.data))
    {
        std::int64_t result = 0;
        const auto parsed = std::from_chars(literal->digits.data(),
            literal->digits.data() + literal->digits.size(), result);
        if (parsed.ec == std::errc{} && parsed.ptr == literal->digits.data() + literal->digits.size())
        {
            return result;
        }
    }
    return std::nullopt;
}

// 有限域证明只接受纯整数表达式与 if/return；不执行外部调用，不用采样推断。
class finite_integer_proof
{
public:
    finite_integer_proof(const function_decl& function, std::size_t overload)
        : function_(function), overload_(overload)
    {
    }

    std::optional<interval> prove()
    {
        interval result{INT64_MAX, INT64_MIN};
        try
        {
            for (std::int64_t input = 0; input <= bound; ++input)
            {
                const auto value = evaluate(input);
                result.first = std::min(result.first, value);
                result.second = std::max(result.second, value);
            }
            return recursive_ ? std::optional(result) : std::nullopt;
        }
        catch (bool)
        {
            return std::nullopt;
        }
    }

private:
    std::int64_t evaluate(std::int64_t input)
    {
        if (input < 0 || input > bound || states_[input] == 1 || ++steps_ > 100000)
        {
            throw false;
        }
        if (states_[input] == 2)
        {
            return values_[input];
        }
        states_[input] = 1;
        const auto result = body(function_.body, input);
        if (!result)
        {
            throw false;
        }
        states_[input] = 2;
        return values_[input] = *result;
    }

    std::optional<std::int64_t> body(const std::vector<stmt_ptr>& statements, std::int64_t input)
    {
        for (const auto& statement : statements)
        {
            if (const auto* result = std::get_if<return_statement>(&statement->data))
            {
                if (!result->value)
                {
                    throw false;
                }
                return expression_value(*result->value, input);
            }
            const auto* branch = std::get_if<if_statement>(&statement->data);
            if (!branch)
            {
                throw false;
            }
            if (auto result = body(expression_value(*branch->condition, input)
                ? branch->then_body : branch->else_body, input))
            {
                return result;
            }
        }
        return std::nullopt;
    }

    std::int64_t expression_value(const expression& item, std::int64_t input)
    {
        if (++steps_ > 100000)
        {
            throw false;
        }
        if (auto value = literal_value(item))
        {
            return *value;
        }
        if (const auto* name = std::get_if<name_reference>(&item.data);
            name && !name->function_value && name->name == function_.parameters[0].name)
        {
            return input;
        }
        if (const auto* call = std::get_if<call_expression>(&item.data))
        {
            if (call->indirect || call->receiver || call->name != function_.name ||
                call->overload_index != overload_ || call->arguments.size() != 1 ||
                call->arguments[0].kind != argument_kind::positional)
            {
                throw false;
            }
            recursive_ = true;
            return evaluate(expression_value(*call->arguments[0].value, input));
        }
        const auto* binary = std::get_if<binary_operation>(&item.data);
        if (!binary || binary->binding)
        {
            throw false;
        }
        const auto left = expression_value(*binary->left, input);
        const auto right = expression_value(*binary->right, input);
        std::int64_t result = 0;
        bool overflow = false;
        switch (binary->operation)
        {
        case token_kind::plus: overflow = __builtin_add_overflow(left, right, &result); break;
        case token_kind::minus: overflow = __builtin_sub_overflow(left, right, &result); break;
        case token_kind::star: overflow = __builtin_mul_overflow(left, right, &result); break;
        case token_kind::slash:
            if (!right || (left == INT64_MIN && right == -1))
            {
                throw false;
            }
            return left / right;
        case token_kind::equal_equal: return left == right;
        case token_kind::bang_equal: return left != right;
        case token_kind::less: return left < right;
        case token_kind::less_equal: return left <= right;
        case token_kind::greater: return left > right;
        case token_kind::greater_equal: return left >= right;
        default: throw false;
        }
        if (overflow)
        {
            throw false;
        }
        return result;
    }

    const function_decl& function_;
    std::size_t overload_;
    std::array<std::int64_t, bound + 1> values_{};
    std::array<std::uint8_t, bound + 1> states_{};
    std::size_t steps_ = 0;
    bool recursive_ = false;
};
}

std::optional<llvm_code_generator::integer_interval>
llvm_code_generator::bounded_integer_result(const function_decl& function)
{
    if (auto found = bounded_integer_results_.find(&function); found != bounded_integer_results_.end())
    {
        return found->second;
    }
    std::optional<integer_interval> result;
    if (!function.external && !function.is_async && function.owner_class.empty() &&
        function.return_type == value_type::int_type && function.parameters.size() == 1 &&
        function.parameters[0].type == value_type::int_type &&
        function.parameters[0].kind == parameter_kind::ordinary && !parameter_is_nullable(function.parameters[0]))
    {
        const auto& overloads = functions_.at(function.name);
        const auto overload = static_cast<std::size_t>(std::find(overloads.begin(), overloads.end(), &function) -
                                                       overloads.begin());
        result = finite_integer_proof(function, overload).prove();
    }
    bounded_integer_results_.emplace(&function, result);
    return result;
}

std::optional<llvm_code_generator::integer_interval>
llvm_code_generator::constant_integer_result(const function_decl& function) const
{
    if (function.external || function.return_type != value_type::int_type)
    {
        return std::nullopt;
    }
    integer_interval range{INT64_MAX, INT64_MIN};
    bool valid = true;
    std::function<void(const std::vector<stmt_ptr>&)> scan = [&](const std::vector<stmt_ptr>& body)
    {
        for (const auto& item : body)
        {
            if (const auto* result = std::get_if<return_statement>(&item->data))
            {
                const auto value = result->value ? literal_value(*result->value) : std::nullopt;
                valid &= value.has_value();
                if (value)
                {
                    range.first = std::min(range.first, *value);
                    range.second = std::max(range.second, *value);
                }
            }
            else if (const auto* branch = std::get_if<if_statement>(&item->data))
            {
                scan(branch->then_body);
                scan(branch->else_body);
            }
            else
            {
                // 不支持的控制流可能包含其他 return，不能遗漏它们。
                valid = false;
            }
        }
    };
    scan(function.body);
    return valid && range.first <= range.second ? std::optional(range) : std::nullopt;
}

} // namespace tx
