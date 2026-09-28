#include "frontend/sema/sema.hpp"

#include <string>
#include <vector>

namespace tx
{
value_type semantic_analyzer::check_channel_intrinsic(
    expression& item, call_expression& call, std::string_view name)
{
    for (const auto& argument : call.arguments)
    {
        if (argument.kind != argument_kind::positional)
        {
            throw compile_error(argument.position,
                "channel 的类型化操作只接受位置实参");
        }
    }
    const auto actual = check_call_arguments(call);
    const auto fail = [&]() -> void
    {
        throw compile_error(item.position,
            "channel." + std::string(name) + " 的类型或参数不匹配");
    };
    if (name == "bounded")
    {
        if (actual != std::vector<value_type>{value_type::int_type})
        {
            fail();
        }
        value_type element = value_type::unknown_type;
        if (call.explicit_type)
        {
            element = *call.explicit_type;
        }
        else if (call.expected_result && call.expected_result->is_channel())
        {
            element = call.expected_result->parameters.front();
        }
        if (!is_send_type(element))
        {
            throw compile_error(item.position,
                "channel.bounded 需要显式 Send 类型实参或声明目标类型");
        }
        call.overload_index = 0;
        return value_type::container_of("channel", {element});
    }
    if (call.explicit_type)
    {
        throw compile_error(item.position,
            "只有 channel.bounded 可以写显式类型实参");
    }
    if (actual.empty())
    {
        fail();
    }
    value_type element = value_type::unknown_type;
    if (name == "select")
    {
        if (actual.front().is_vector() &&
            actual.front().parameters.front().is_channel())
        {
            element = actual.front().parameters.front().parameters.front();
        }
    }
    else if (actual.front().is_channel())
    {
        element = actual.front().parameters.front();
    }
    if (!is_send_type(element))
    {
        fail();
    }
    const auto channel = value_type::container_of("channel", {element});
    const auto token = value_type::cancel_token_type;
    value_type result = value_type::void_type;
    if (name == "send")
    {
        if (actual != std::vector<value_type>{channel, element,
                value_type::int_type, token})
        {
            fail();
        }
        const auto& message = *call.arguments[1].value;
        if (requires_unique_move(message.type) &&
            (!is_uniquely_owned(message) ||
             (!is_move_expression(message) &&
              !is_fresh_owned_expression(message))))
        {
            throw compile_error(message.position,
                "可变 Send 消息需要唯一 move 或新构造值");
        }
        result = value_type::bool_type;
    }
    else if (name == "recv")
    {
        if (actual != std::vector<value_type>{channel,
                value_type::int_type, token})
        {
            fail();
        }
        result = value_type::container_of("option", {element});
    }
    else if (name == "close")
    {
        if (actual != std::vector<value_type>{channel})
        {
            fail();
        }
        result = value_type::bool_type;
    }
    else if (name == "select")
    {
        if (actual != std::vector<value_type>{
                value_type::vector_of(channel), value_type::int_type, token})
        {
            fail();
        }
        result = value_type::container_of("selected", {element});
    }
    else
    {
        fail();
    }
    call.overload_index = 0;
    return result;
}

} // namespace tx
