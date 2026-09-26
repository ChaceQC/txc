#include "frontend/sema/sema.hpp"

#include <string>
#include <string_view>

namespace tx
{

value_type semantic_analyzer::check_random_intrinsic(
    expression& item, call_expression& call, std::string_view name)
{
    for (const auto& argument : call.arguments)
    {
        if (argument.kind != argument_kind::positional)
        {
            throw compile_error(argument.position,
                "random 的类型化操作只接受位置实参");
        }
    }
    const auto actual = check_call_arguments(call);
    const auto count = name == "sample" ? 3U : 2U;
    if (actual.size() != count || actual[0] != random_generator_type_ ||
        !actual[1].is_vector() ||
        (name == "sample" && actual[2] != value_type::int_type))
    {
        throw compile_error(item.position,
            "random." + std::string(name) +
            " 需要 generator、具体 vector<T>" +
            (name == "sample" ? " 和 int 数量" : ""));
    }
    call.overload_index = 0;
    return name == "choice" ? actual[1].parameters.front() : actual[1];
}

} // namespace tx
