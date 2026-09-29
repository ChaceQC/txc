#include "stdlib/db_async.hpp"
#include "stdlib/db_internal.hpp"

namespace tx_generated
{
namespace
{

struct operation_guard
{
    const db_operation_context* previous = db_current_operation;

    explicit operation_guard(const db_operation_context& operation)
    {
        db_current_operation = &operation;
    }
    ~operation_guard()
    {
        db_current_operation = previous;
    }
};

std::vector<db_row> read_rows(const db_statement& statement, const db_async_request& request)
{
    auto cursor = db_query(statement);
    std::vector<db_row> rows;
    std::size_t bytes = 0;
    while (auto row = db_next(cursor))
    {
        db_operation_check();
        bytes += sizeof(db_row_data);
        for (const auto& name : row->names)
        {
            bytes += sizeof(std::string) + name.size();
        }
        for (const auto& value : row->values)
        {
            bytes += db_value_size(value);
        }
        if (rows.size() >= static_cast<std::size_t>(request.max_rows) ||
            bytes > static_cast<std::size_t>(request.max_bytes))
        {
            db_fail("limit_exceeded", "异步查询结果超过行数或字节预算");
        }
        rows.push_back(std::move(row));
    }
    return rows;
}

} // namespace

std::size_t db_value_size(const db_value& value)
{
    if (const auto* text = std::get_if<std::string>(&value.data))
    {
        return sizeof(db_value) + text->size();
    }
    if (const auto* bytes = std::get_if<byte_value>(&value.data))
    {
        return sizeof(db_value) + static_cast<std::size_t>(bytes_length(*bytes));
    }
    return sizeof(db_value);
}

db_async_result db_run_async(const db_async_request& request)
{
    operation_guard operation(request.operation);
    db_operation_check();
    auto connection = db_acquire(request.pool, db_operation_remaining(2147483647));
    db_operation_check();
    // 借用从创建到清理始终在此工作线程，绝不改写同步连接的 owner。
    auto transaction = db_begin(connection, connection->pg_driver ? "read_committed" : "deferred");
    auto statement = db_prepare(connection, request.sql);
    db_bind_all(statement, request.parameters);
    db_operation_check();
    db_async_result result;
    if (request.query)
    {
        result = read_rows(statement, request);
    }
    else
    {
        result = db_execute(statement);
    }
    db_operation_check();
    db_commit(transaction);
    // COMMIT 已确认后只做析构归还，清理失败丢弃连接而不改写成功。
    return result;
}

} // namespace tx_generated
