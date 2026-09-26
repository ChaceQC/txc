#include "frontend/sema/sema.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace tx
{

value_type semantic_analyzer::check_algorithm_intrinsic(
    expression& item, call_expression& call, std::string_view name)
{
    for (const auto& argument : call.arguments)
    {
        if (argument.kind != argument_kind::positional)
        {
            throw compile_error(argument.position,
                "algorithm 的类型化操作只接受位置实参");
        }
    }
    const auto actual = check_call_arguments(call);
    const auto fail = [&](std::string_view detail) -> void
    {
        throw compile_error(item.position,
            "algorithm." + std::string(name) + "：" + std::string(detail));
    };
    if (actual.empty() || !actual.front().is_vector())
    {
        fail("第一个参数必须是具体 vector<T>");
    }
    const auto& element = actual.front().parameters.front();
    const auto compare = value_type::function_of(
        {element, element}, value_type::int_type);
    const auto predicate = value_type::function_of(
        {element}, value_type::bool_type);
    const auto needs_order = name == "stable_sort" ||
        name == "stable_sorted" || name == "binary_search" ||
        name == "equal_range" || name == "unique";
    if (needs_order)
    {
        const bool has_key = name == "binary_search" || name == "equal_range";
        const std::size_t required = has_key ? 2 : 1;
        if ((actual.size() != required && actual.size() != required + 1) ||
            (has_key && actual[1] != element) ||
            (actual.size() == required + 1 && actual.back() != compare))
        {
            fail("需要 vector<T>、可选的同类型键及 fn(T,T)->int 比较器");
        }
        if (actual.size() == required)
        {
            bool ordered = element == value_type::int_type ||
                element == value_type::float_type ||
                element == value_type::bool_type ||
                element == value_type::str_type;
            if (const auto found = structs_.find(element.name);
                found != structs_.end())
            {
                validate_ordered_key_shape(element, item.position);
                for (const auto& method : found->second->methods)
                {
                    ordered |= method.operator_kind == token_kind::less &&
                        method.parameters.size() == 1 &&
                        method.parameters.front().type == element &&
                        method.return_type == value_type::bool_type;
                }
            }
            if (!ordered)
            {
                fail("元素没有默认顺序，请传入 fn(T,T)->int 比较器");
            }
        }
    }
    else if (name == "rotate")
    {
        if (actual.size() != 2 || actual[1] != value_type::int_type)
        {
            fail("需要 vector<T> 和 int middle");
        }
    }
    else if (name == "partition" || name == "filter" ||
             name == "all" || name == "any")
    {
        if (actual.size() != 2 || actual[1] != predicate)
        {
            fail("谓词必须是 fn(T)->bool");
        }
    }
    else if (name == "map")
    {
        if (actual.size() != 2 || !actual[1].is_function() ||
            actual[1].parameters.size() != 2 ||
            actual[1].parameters.front() != element)
        {
            fail("变换函数必须是 fn(T)->U");
        }
        validate_type(value_type::vector_of(actual[1].parameters.back()),
            call.arguments[1].position);
    }
    else if (name == "fold")
    {
        if (actual.size() != 3 || actual[1] == value_type::void_type ||
            actual[1] == value_type::any_type ||
            actual[2] != value_type::function_of(
                {actual[1], element}, actual[1]))
        {
            fail("需要具体初值 U 和 fn(U,T)->U 累计函数");
        }
        validate_type(value_type::vector_of(actual[1]),
            call.arguments[1].position);
    }
    else
    {
        fail("未知内置算法");
    }
    const auto& overloads = functions_.at(call.name);
    bool matched = false;
    for (std::size_t index = 0; index < overloads.size(); ++index)
    {
        if (overloads[index].parameters.size() == actual.size())
        {
            call.overload_index = index;
            matched = true;
            break;
        }
    }
    if (!matched)
    {
        fail("公开接口缺少对应参数个数的声明");
    }
    if (name == "stable_sorted" || name == "filter")
    {
        return actual.front();
    }
    if (name == "map")
    {
        return value_type::vector_of(actual[1].parameters.back());
    }
    if (name == "fold")
    {
        return actual[1];
    }
    if (name == "binary_search")
    {
        return value_type::container_of("option", {value_type::int_type});
    }
    if (name == "equal_range")
    {
        return value_type::vector_of(value_type::int_type);
    }
    if (name == "partition" || name == "unique")
    {
        return value_type::int_type;
    }
    if (name == "all" || name == "any")
    {
        return value_type::bool_type;
    }
    return value_type::void_type;
}

} // namespace tx
