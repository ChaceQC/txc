#include "frontend/sema/sema.hpp"

#include <string>

namespace tx
{
namespace
{

bool send_scalar(const value_type& type)
{
    return type == value_type::int_type || type == value_type::float_type ||
           type == value_type::bool_type;
}

value_type make(const std::string& kind, const value_type& element)
{
    return value_type::container_of(kind, {element});
}

} // namespace

value_type semantic_analyzer::check_sync_intrinsic(
    expression& item, call_expression& call, std::string_view name)
{
    for (const auto& argument : call.arguments)
    {
        if (argument.kind != argument_kind::positional)
        {
            throw compile_error(argument.position,
                "sync 的类型化操作只接受位置实参");
        }
    }
    const auto actual = check_call_arguments(call);
    const auto fail = [&]() -> void
    {
        throw compile_error(item.position,
            "sync." + std::string(name) + " 的类型或参数不匹配");
    };
    if (actual.empty())
    {
        fail();
    }
    const auto& first = actual.front();
    const bool creates = name == "new_mutex" || name == "new_rw_lock" ||
                         name == "new_atomic";
    const auto element = creates ? first :
        name == "wait" && actual.size() > 1 && actual[1].is_sync_value()
            ? actual[1].parameters.front() :
        first.is_sync_value() ? first.parameters.front() : value_type::unknown_type;
    if ((name == "new_atomic" || name.starts_with("atomic_"))
            ? !send_scalar(element) : !is_send_type(element))
    {
        throw compile_error(call.arguments.front().position,
            "同步容器需要已证明 Send 的值；atomic 仅支持 int/bool");
    }
    value_type result = value_type::void_type;
    if (creates)
    {
        if (actual.size() != 1 ||
            (name == "new_atomic" && element == value_type::float_type))
        {
            fail();
        }
        if (name != "new_atomic" && requires_unique_move(element))
        {
            const auto& initial = *call.arguments.front().value;
            if (!is_uniquely_owned(initial) ||
                (!is_move_expression(initial) &&
                 !is_fresh_owned_expression(initial)))
            {
                throw compile_error(initial.position,
                    "受锁保护的可变初值需要唯一 move 或新构造值");
            }
        }
        result = make(name.substr(4) == "rw_lock" ? "rw_lock" :
            name.substr(4) == "atomic" ? "atomic" : "mutex", element);
    }
    else if (name == "lock" || name == "read_lock" || name == "write_lock")
    {
        const auto owner = name == "lock" ? "mutex" : "rw_lock";
        if (actual.size() != 1 || first != make(owner, element))
        {
            fail();
        }
        result = make(name == "lock" ? "mutex_guard" :
            name == "read_lock" ? "rw_read_guard" : "rw_write_guard", element);
    }
    else if (name == "guard_get" || name == "guard_set" ||
             name == "guard_close" || name == "read_get" ||
             name == "write_get" || name == "write_set" ||
             name == "read_close" || name == "write_close")
    {
        const auto guard_kind = name.starts_with("guard_") ? "mutex_guard" :
            name.starts_with("read_") ? "rw_read_guard" : "rw_write_guard";
        const bool setter = name.ends_with("_set");
        if (first != make(guard_kind, element) ||
            actual.size() != (setter ? 2U : 1U) ||
            (setter && actual[1] != element))
        {
            fail();
        }
        if (setter && requires_unique_move(element))
        {
            const auto& next = *call.arguments[1].value;
            if (!is_uniquely_owned(next) ||
                (!is_move_expression(next) &&
                 !is_fresh_owned_expression(next)))
            {
                throw compile_error(next.position,
                    "受锁保护的可变新值需要唯一 move 或新构造值");
            }
        }
        if (name.ends_with("_get"))
        {
            result = element;
        }
    }
    else if (name == "wait")
    {
        if (actual.size() != 4 || first.name != "condition" ||
            actual[1] != make("mutex_guard", element) ||
            actual[2] != value_type::int_type ||
            actual[3] != value_type::cancel_token_type)
        {
            fail();
        }
        result = value_type::bool_type;
    }
    else if (name.starts_with("atomic_"))
    {
        if (first != make("atomic", element) ||
            (element != value_type::int_type && element != value_type::bool_type))
        {
            fail();
        }
        if (name == "atomic_load")
        {
            if (actual != std::vector<value_type>{first, value_type::int_type})
            {
                fail();
            }
            result = element;
        }
        else if (name == "atomic_store" || name == "atomic_exchange" ||
                 name == "atomic_fetch_add")
        {
            if (actual != std::vector<value_type>{first, element,
                    value_type::int_type} ||
                (name == "atomic_fetch_add" && element != value_type::int_type))
            {
                fail();
            }
            if (name != "atomic_store")
            {
                result = element;
            }
        }
        else if (name == "atomic_compare_exchange")
        {
            if (actual != std::vector<value_type>{first, element, element,
                    value_type::int_type})
            {
                fail();
            }
            result = value_type::bool_type;
        }
        else
        {
            fail();
        }
    }
    else
    {
        fail();
    }
    call.overload_index = 0;
    return result;
}

} // namespace tx
