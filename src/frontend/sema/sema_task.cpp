#include "frontend/sema/sema.hpp"

#include <string>

namespace tx
{
value_type semantic_analyzer::check_task_intrinsic(
    expression& item, call_expression& call, std::string_view name)
{
    for (const auto& argument : call.arguments)
    {
        if (argument.kind != argument_kind::positional)
        {
            throw compile_error(argument.position,
                "task 的类型化操作只接受位置实参");
        }
    }
    const auto actual = check_call_arguments(call);
    const auto fail = [&]() -> void
    {
        throw compile_error(item.position,
            "task." + std::string(name) + " 的类型或参数不匹配");
    };
    call.overload_index = 0;
    if (name == "scope")
    {
        if (actual.size() != 3 || !actual[0].is_function() ||
            actual[0].parameters.size() != 2 ||
            actual[0].parameters[0].name != "task_scope" ||
            (actual[0].parameters[1] != value_type::void_type &&
             !is_send_type(actual[0].parameters[1])) ||
            actual[1] != value_type::int_type ||
            actual[2] != value_type::int_type)
        {
            fail();
        }
        return actual[0].parameters[1];
    }
    if (name == "spawn")
    {
        if (actual.size() != 2 || actual[0].name != "task_scope" ||
            !actual[1].is_function() || actual[1].parameters.size() != 1 ||
            (actual[1].parameters[0] != value_type::void_type &&
             !is_send_type(actual[1].parameters[0])))
        {
            fail();
        }
        if (!thread_callback_is_send(*call.arguments[1].value))
        {
            throw compile_error(call.arguments[1].position,
                "任务闭包必须由顶层函数及直接 bind 的 Send 捕获构成");
        }
        return value_type::container_of("task",
            {actual[1].parameters[0]});
    }
    if (name == "wait")
    {
        if (actual.size() != 1 || !actual[0].is_task())
        {
            fail();
        }
        return actual[0].parameters[0];
    }
    fail();
    return value_type::void_type;
}

} // namespace tx
