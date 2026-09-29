#include "stdlib/db_pool_internal.hpp"

namespace tx_generated
{
namespace
{

db_connection clean_connection(db_connection_state& connection)
{
    if (!connection.postgres || connection.cleanup_failed)
    {
        // SQLite 必须重新打开，防止 temp 表或连接局部配置污染下次借用。
        connection.native.reset();
        connection.postgres.reset();
        return {};
    }
    if (db_in_transaction(connection))
    {
        db_pg_control(connection, "ROLLBACK");
    }
    db_pg_control(connection, "DISCARD ALL");
    db_pg_configure(connection);
    auto idle = std::make_shared<db_connection_state>();
    idle->postgres = std::move(connection.postgres);
    idle->pg_driver = true;
    idle->pg_read_only = connection.pg_read_only;
    idle->operation_timeout_ms = connection.operation_timeout_ms;
    idle->value_limit = connection.value_limit;
    return idle;
}

void return_slot(db_connection_state& connection, bool report)
{
    auto pool = std::move(connection.pool);
    if (!pool)
    {
        return;
    }
    std::exception_ptr error;
    db_connection idle;
    bool closed = false;
    {
        std::lock_guard lock(pool->mutex);
        closed = pool->closed;
    }
    try
    {
        if (!closed)
        {
            idle = clean_connection(connection);
        }
    }
    catch (...)
    {
        error = std::current_exception();
    }
    connection.native.reset();
    connection.postgres.reset();
    connection.cleanup_failed = true;
    {
        std::lock_guard lock(pool->mutex);
        if (idle && !pool->closed)
        {
            pool->idle.push_back(std::move(idle));
        }
        else
        {
            --pool->total;
        }
        pool->changed.notify_one();
    }
    if (error && report)
    {
        std::rethrow_exception(error);
    }
}

} // namespace

void db_return_lease(db_connection_state& connection) noexcept
{
    try
    {
        return_slot(connection, false);
    }
    catch (...)
    {
        // 析构期间不抛出外部数据库错误。
    }
}

bool db_release(const db_connection& connection)
{
    db_check_thread(connection);
    if (!connection->pool)
    {
        if (!db_connection_open(*connection))
        {
            return false;
        }
        db_fail("invalid_state", "该连接不是连接池借用");
    }
    db_abort_cursor(connection);
    for (const auto& weak : connection->statements)
    {
        if (const auto statement = weak.lock())
        {
            statement->native.reset();
            statement->pg_open = false;
            statement->bound.clear();
        }
    }
    if (const auto transaction = connection->transaction.lock())
    {
        transaction->active = false;
        transaction->savepoints.clear();
    }
    connection->statements.clear();
    connection->transaction.reset();
    return_slot(*connection, true);
    return true;
}

} // namespace tx_generated
