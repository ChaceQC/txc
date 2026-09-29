#include "backend/cpp/db_abi.hpp"
#include "backend/cpp/db_abi_internal.hpp"
#include "backend/cpp/task_executor.hpp"
#include "backend/cpp/task_runtime_internal.hpp"
#include "stdlib/db_async.hpp"

using namespace tx_generated;
using namespace tx_generated::db_abi;

namespace
{

std::any result_value(db_async_result result, const std::string& type_name)
{
    if (const auto* rows = std::get_if<std::vector<db_row>>(&result))
    {
        object_vector values(type_name);
        values.data().values.reserve(rows->size());
        for (const auto& row : *rows)
        {
            values.data().values.emplace_back(row);
        }
        values.data().refresh();
        return values;
    }
    const auto& executed = std::get<db_execution>(result);
    struct_fields id_fields(2);
    id_fields[0] = {"present", executed.insert_id.has_value()};
    id_fields[1] = {"value", executed.insert_id ? std::any(*executed.insert_id) : std::any{}};
    struct_fields fields(2);
    fields[0] = {"affected_rows", executed.affected_rows};
    fields[1] = {"insert_id", dynamic_struct(dynamic_struct_data{
        "option<int>", "option", std::move(id_fields)})};
    return dynamic_struct(dynamic_struct_data{type_name, "execution", std::move(fields)});
}

void run_request(const db_async_request& request, const std::shared_ptr<task_state>& child,
    const std::string& value_type, const std::vector<detail::source_frame>& stack) noexcept
{
    detail::runtime_context context;
    auto* previous = detail::thread_context;
    context.concurrent_depth = previous ? previous->concurrent_depth : 0;
    detail::thread_context = &context;
    task_result result;
    task_error error;
    {
        concurrent_execution_scope execution;
        try
        {
            result = result_value(db_run_async(request), value_type);
        }
        catch (const runtime_failure& failure)
        {
            const auto& info = failure.error();
            error = {info.kind, info.code, info.message, stack};
        }
        catch (...)
        {
            error = {tx::error_kind::database, "query_failed", "异步数据库操作失败", stack};
        }
        complete_task(child, std::move(result), std::move(error));
    }
    detail::thread_context = previous;
}

void* schedule(const void* pool, const void* sql, const void* params,
    std::int64_t max_rows, std::int64_t max_bytes, std::int64_t timeout_ms,
    const void* token, const char* task_type, const char* value_type, bool query)
{
    const auto scope = current_task_scope();
    if (!scope || timeout_ms < 1 || timeout_ms > 2147483647 ||
        (query && (max_rows < 1 || max_rows > 1000000 || max_bytes < 1 || max_bytes > 67108864)))
    {
        db_fail("invalid_argument", "异步数据库需要活动任务作用域、有效超时及结果限额");
    }
    auto request = std::make_shared<db_async_request>();
    request->pool = input<db_pool>(pool);
    request->sql = detail::text_value(sql);
    request->operation = {input<cancel_token>(token).state, scope->cancellation,
        std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms)};
    if (!request->operation.token || request->sql.empty() || request->sql.size() > 1048576)
    {
        db_fail("invalid_argument", "异步数据库取消令牌或 SQL 无效");
    }
    const auto& values = field<object_vector>(input<dynamic_struct>(params), 0).data().values;
    if (values.size() > 4096)
    {
        db_fail("limit_exceeded", "异步数据库参数过多");
    }
    std::size_t bytes = 0;
    for (const auto& value : values)
    {
        const auto& parameter = std::any_cast<const db_value&>(value);
        bytes += db_value_size(parameter);
        if (bytes > 67108864)
        {
            db_fail("limit_exceeded", "异步数据库参数超过字节预算");
        }
        request->parameters.push_back(parameter);
    }
    request->query = query;
    request->max_rows = max_rows;
    request->max_bytes = max_bytes;
    auto child = reserve_task(scope);
    void* handle = nullptr;
    try
    {
        handle = detail::make_handle<std::any>(task_handle{child, 5, task_type});
        enqueue_task([request, child, name = std::string(value_type),
            stack = detail::capture_stack(detail::current_runtime_context())]
        {
            run_request(*request, child, name, stack);
        });
    }
    catch (...)
    {
        if (handle)
        {
            detail::destroy_handle(static_cast<std::any*>(handle));
        }
        discard_task(child);
        throw;
    }
    return handle;
}

} // namespace

extern "C" int txrt_db_query_async(const void* pool, const void* sql, const void* params,
    std::int64_t max_rows, std::int64_t max_bytes, std::int64_t timeout_ms, const void* token,
    const char* task_type, const char* value_type, void** result) noexcept
{
    return detail::invoke_checked([&]
    {
        *result = schedule(pool, sql, params, max_rows, max_bytes, timeout_ms,
            token, task_type, value_type, true);
    }, tx::error_kind::database);
}

extern "C" int txrt_db_execute_async(const void* pool, const void* sql, const void* params,
    std::int64_t timeout_ms, const void* token, const char* task_type,
    const char* value_type, void** result) noexcept
{
    return detail::invoke_checked([&]
    {
        *result = schedule(pool, sql, params, 0, 0, timeout_ms, token, task_type, value_type, false);
    }, tx::error_kind::database);
}
