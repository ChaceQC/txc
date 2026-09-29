#include "frontend/sema/sema.hpp"

#include <functional>
#include <string>
#include <unordered_set>

namespace tx
{
bool semantic_analyzer::is_lock_guard_type(const value_type& type) const
{
    if (!type.is_sync_value())
    {
        return false;
    }
    const auto kind = type.container_name();
    return kind == "mutex_guard" || kind == "rw_read_guard" ||
           kind == "rw_write_guard";
}

bool semantic_analyzer::is_send_type(const value_type& type) const
{
    std::unordered_set<std::string> visiting;
    const auto check = [&](const auto& self, const value_type& candidate) -> bool
    {
        if (candidate == value_type::int_type ||
            candidate == value_type::float_type ||
            candidate == value_type::bool_type ||
            candidate == value_type::str_type ||
            candidate == value_type::bytes_type ||
            candidate == value_type::none_type ||
            candidate == value_type::cancel_token_type ||
            candidate.name == "condition" || candidate.name == "semaphore" ||
            candidate.name == "once" || candidate.name == "db_pool" ||
            candidate.name == "db_row" || candidate.name == "db_value")
        {
            return true;
        }
        if (is_lock_guard_type(candidate) || candidate.is_iterator() ||
            candidate.is_join_handle() || candidate.is_task() ||
            candidate.name == "task_scope" || candidate == value_type::any_type ||
            candidate == value_type::array_type ||
            candidate == value_type::dict_type ||
            candidate == value_type::secret_bytes_type ||
            candidate == value_type::ipc_listener_type ||
            candidate == value_type::ipc_stream_type ||
            candidate.is_database_type() ||
            candidate.is_function())
        {
            return false;
        }
        if (candidate.is_sync_value())
        {
            const auto kind = candidate.container_name();
            if (candidate.parameters.size() != 1 ||
                (kind != "mutex" && kind != "rw_lock" && kind != "atomic"))
            {
                return false;
            }
            if (kind == "atomic")
            {
                return candidate.parameters.front() == value_type::int_type ||
                       candidate.parameters.front() == value_type::bool_type;
            }
            return self(self, candidate.parameters.front());
        }
        if (candidate.is_channel())
        {
            return candidate.parameters.size() == 1 &&
                   self(self, candidate.parameters.front());
        }
        if (candidate.is_option() || candidate.is_result() ||
            candidate.is_selected() ||
            candidate.is_vector() || candidate.is_deque() ||
            candidate.is_priority_entry())
        {
            return candidate.parameters.size() == 1 &&
                   self(self, candidate.parameters.front());
        }
        if (candidate.is_map() || candidate.is_entry())
        {
            return candidate.parameters.size() == 2 &&
                   self(self, candidate.parameters[0]) &&
                   self(self, candidate.parameters[1]);
        }
        if (candidate.is_typed_container())
        {
            for (const auto& parameter : candidate.parameters)
            {
                if (!self(self, parameter))
                {
                    return false;
                }
            }
            return true;
        }
        if (const auto found = structs_.find(candidate.name);
            found != structs_.end())
        {
            const auto& file = found->second->position.file;
            const auto separator = file.find_last_of("/\\");
            const auto filename = file.substr(separator == std::string::npos
                ? 0 : separator + 1);
            if (filename == "socket.txh" &&
                (candidate.name.ends_with("_tcp_listener") ||
                 candidate.name.ends_with("_tcp_stream") ||
                 candidate.name.ends_with("_udp_socket")))
            {
                return false;
            }
            if (filename == "tls.txh" &&
                candidate.name.ends_with("_secure_stream"))
            {
                return false;
            }
            if (!visiting.insert(candidate.name).second)
            {
                return true;
            }
            for (const auto& field : found->second->fields)
            {
                if (!self(self, field.type))
                {
                    visiting.erase(candidate.name);
                    return false;
                }
            }
            visiting.erase(candidate.name);
            return true;
        }
        // 未登记的不透明句柄、class 与未知类型默认不跨线程。
        return false;
    };
    return check(check, type);
}

bool semantic_analyzer::is_sync_type(const value_type& type) const
{
    if (const auto found = structs_.find(type.name);
        found != structs_.end())
    {
        const auto& file = found->second->position.file;
        const auto separator = file.find_last_of("/\\");
        const auto filename = file.substr(separator == std::string::npos
            ? 0 : separator + 1);
        if (filename == "httpx.txh" &&
            (type.name.ends_with("_listener") ||
             type.name.ends_with("_client_session")))
        {
            return true;
        }
        if (filename == "websocket.txh" &&
            type.name.ends_with("_connection"))
        {
            // WS 收发句柄可唯一移动到工作线程，但不能复制后并发操作。
            return false;
        }
    }
    if (type == value_type::int_type || type == value_type::float_type ||
        type == value_type::bool_type || type == value_type::str_type ||
        type == value_type::bytes_type || type == value_type::none_type ||
        type == value_type::cancel_token_type || type.name == "condition" ||
        type.name == "semaphore" || type.name == "once" || type.name == "db_pool" ||
        type.name == "db_row" || type.name == "db_value")
    {
        return true;
    }
    if (is_lock_guard_type(type))
    {
        return false;
    }
    if (type.is_sync_value())
    {
        const auto kind = type.container_name();
        if (kind == "mutex" || kind == "rw_lock")
        {
            return type.parameters.size() == 1 &&
                   is_send_type(type.parameters.front());
        }
        if (kind == "atomic")
        {
            return type.parameters.size() == 1 &&
                (type.parameters.front() == value_type::int_type ||
                 type.parameters.front() == value_type::bool_type);
        }
    }
    if (type.is_channel())
    {
        return is_send_type(type);
    }
    if (type.is_option() || type.is_result() || type.is_selected())
    {
        return type.parameters.size() == 1 &&
               is_sync_type(type.parameters.front());
    }
    return false;
}

bool semantic_analyzer::requires_unique_move(const value_type& type) const
{
    if (!is_send_type(type) || is_sync_type(type))
    {
        return false;
    }
    if (type.is_option() || type.is_result() || type.is_selected())
    {
        return !type.parameters.empty() &&
               requires_unique_move(type.parameters.front());
    }
    return type.is_vector() || type.is_typed_container() || type.is_map() ||
           type.is_entry() || type.is_priority_entry() ||
           structs_.contains(type.name);
}

bool semantic_analyzer::is_move_expression(const expression& item) const
{
    const auto* call = std::get_if<call_expression>(&item.data);
    return call != nullptr && call->name == "move" &&
           call->arguments.size() == 1;
}

bool semantic_analyzer::is_fresh_owned_expression(const expression& item) const
{
    if (!requires_unique_move(item.type))
    {
        return false;
    }
    if (item.ownership_id != 0)
    {
        return is_move_expression(item);
    }
    const auto* call = std::get_if<call_expression>(&item.data);
    if (call == nullptr)
    {
        return false;
    }
    if (call->name == "deep_copy")
    {
        return true;
    }
    if (call->overload_index)
    {
        const auto found = functions_.find(call->name);
        if (found != functions_.end())
        {
            const auto& name = found->second.at(*call->overload_index).external_name;
            if (name == "websocket.connect" || name == "websocket.accept" ||
                name == "websocket.upgrade")
            {
                // 这些入口在运行时登记全新的连接句柄，可唯一移交给异步任务。
                return true;
            }
        }
    }
    if (!call->is_constructor && !call->container_type)
    {
        return false;
    }
    for (const auto& argument : call->arguments)
    {
        const auto& value = *argument.value;
        if (value.type == value_type::array_type ||
            value.type == value_type::dict_type ||
            value.type == value_type::any_type)
        {
            return false;
        }
        if (requires_unique_move(value.type) &&
            !is_move_expression(value) &&
            !is_fresh_owned_expression(value))
        {
            return false;
        }
    }
    return true;
}

void semantic_analyzer::mark_expression_escaped(const expression& item)
{
    if (item.ownership_id != 0 && !is_move_expression(item))
    {
        escaped_ownership_.insert(item.ownership_id);
    }
}

bool semantic_analyzer::is_uniquely_owned(const expression& item) const
{
    if (!requires_unique_move(item.type))
    {
        return is_send_type(item.type);
    }
    if (item.ownership_id == 0)
    {
        return is_fresh_owned_expression(item);
    }
    if (escaped_ownership_.contains(item.ownership_id))
    {
        return false;
    }
    std::size_t owners = 0;
    for (const auto& scope : scopes_)
    {
        for (const auto& [name, symbol] : scope)
        {
            (void)name;
            owners += symbol.ownership_id == item.ownership_id &&
                      !symbol.moved ? 1U : 0U;
        }
    }
    return owners == (is_move_expression(item) ? 0U : 1U);
}

bool semantic_analyzer::thread_callback_is_send(
    const expression& item, bool allow_unique_move) const
{
    if (const auto* name = std::get_if<name_reference>(&item.data))
    {
        return name->function_value;
    }
    const auto* binding = std::get_if<call_expression>(&item.data);
    if (!binding || binding->name != "bind" || binding->arguments.size() < 2 ||
        !thread_callback_is_send(*binding->arguments.front().value,
                                 allow_unique_move))
    {
        return false;
    }
    for (std::size_t index = 1; index < binding->arguments.size(); ++index)
    {
        const auto& captured = *binding->arguments[index].value;
        if (!is_send_type(captured.type))
        {
            return false;
        }
        if (!requires_unique_move(captured.type))
        {
            continue;
        }
        if (!allow_unique_move ||
            (!is_move_expression(captured) &&
             !is_fresh_owned_expression(captured)) ||
            !is_uniquely_owned(captured))
        {
            return false;
        }
    }
    return true;
}

value_type semantic_analyzer::check_thread_intrinsic(
    expression& item, call_expression& call, std::string_view name)
{
    if (call.arguments.size() != 1 ||
        call.arguments.front().kind != argument_kind::positional)
    {
        throw compile_error(item.position,
            "thread." + std::string(name) + " 需要一个位置实参");
    }
    const auto& argument = *call.arguments.front().value;
    const auto actual = check_expression(*call.arguments.front().value);
    call.overload_index = 0;
    if (name == "spawn")
    {
        if (!actual.is_function() || actual.parameters.size() != 1 ||
            (actual.parameters.back() != value_type::void_type &&
             !is_send_type(actual.parameters.back())))
        {
            throw compile_error(argument.position,
                "thread.spawn 需要返回 void 或 Send 类型的 fn()->T");
        }
        if (!thread_callback_is_send(argument))
        {
            throw compile_error(argument.position,
                "跨线程闭包必须由顶层函数与 Send 捕获构成；可变复合捕获需唯一 move");
        }
        return value_type::container_of("join_handle",
            {actual.parameters.back()});
    }
    if (!actual.is_join_handle())
    {
        throw compile_error(argument.position,
            "thread." + std::string(name) + " 需要 join_handle<T>");
    }
    if (name == "join")
    {
        return actual.parameters.front();
    }
    if (name == "id")
    {
        return value_type::int_type;
    }
    if (name == "detach")
    {
        return value_type::void_type;
    }
    throw compile_error(item.position, "未知 thread 接口");
}

} // namespace tx
