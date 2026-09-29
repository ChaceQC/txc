#include "stdlib/db_internal.hpp"
#include "stdlib/db_operation.hpp"

namespace tx_generated
{

bool db_connection_open(const db_connection_state& connection) noexcept
{
    return connection.native || connection.postgres;
}

bool db_statement_open(const db_statement_state& statement) noexcept
{
    return statement.native || statement.pg_open;
}

bool db_in_transaction(const db_connection_state& connection) noexcept
{
    if (connection.postgres)
    {
        const auto status = PQtransactionStatus(connection.postgres.get());
        return status == PQTRANS_INTRANS || status == PQTRANS_INERROR;
    }
    return connection.native && !sqlite3_get_autocommit(connection.native.get());
}

db_connection_state::~db_connection_state()
{
    db_return_lease(*this);
}

db_statement_state::~db_statement_state()
{
    if (pg_open && connection->postgres)
    {
        try
        {
            connection->retired_statements.push_back(std::move(pg_name));
        }
        catch (...)
        {
            connection->cleanup_failed = true;
        }
    }
}

void db_connection_deleter::operator()(sqlite3* value) const noexcept
{
    sqlite3_close_v2(value);
}

void db_statement_deleter::operator()(sqlite3_stmt* value) const noexcept
{
    sqlite3_finalize(value);
}

void db_check_thread(const db_connection& connection)
{
    if (!connection)
    {
        db_fail("invalid_state", "数据库连接句柄无效");
    }
    if (connection->owner != std::this_thread::get_id())
    {
        db_fail("thread_violation", "数据库连接只能在创建线程使用");
    }
}

void db_check_connection(const db_connection& connection, bool idle)
{
    db_check_thread(connection);
    if (!db_connection_open(*connection) || connection->cleanup_failed)
    {
        db_fail("invalid_state", "数据库连接已关闭或自动回滚失败");
    }
    if (idle && !connection->cursor.expired())
    {
        db_fail("invalid_state", "数据库连接仍有活动游标");
    }
    if (idle && connection->postgres &&
        PQtransactionStatus(connection->postgres.get()) == PQTRANS_INERROR)
    {
        db_fail("transaction_failed", "PostgreSQL 事务已失败，必须回滚或回滚到保存点");
    }
}

void db_check_statement(const db_statement& statement, bool idle)
{
    if (!statement || !db_statement_open(*statement))
    {
        db_fail("invalid_state", "数据库语句已关闭");
    }
    db_check_connection(statement->connection, idle);
}

void db_check_bound(const db_statement& statement)
{
    for (const auto& value : statement->bound)
    {
        if (!value)
        {
            db_fail("parameter_missing", "数据库语句仍有未绑定参数");
        }
    }
}

void db_reconcile_transaction(const db_connection& connection)
{
    if (!db_in_transaction(*connection))
    {
        if (const auto transaction = connection->transaction.lock())
        {
            transaction->active = false;
            transaction->savepoints.clear();
        }
        connection->transaction.reset();
    }
}

[[noreturn]] void db_sqlite_failure(const db_connection& connection,
                                    int status, bool opening)
{
    int code = connection->native
        ? sqlite3_extended_errcode(connection->native.get()) : status;
    if ((code & 255) != (status & 255))
    {
        code = status;
    }
    connection->last_error = code;
    db_reconcile_transaction(connection);
    if ((code & 255) == SQLITE_INTERRUPT || (code & 255) == SQLITE_BUSY ||
        (code & 255) == SQLITE_LOCKED)
    {
        db_operation_check();
    }
    const char* stable = opening ? "connection_failed" : "query_failed";
    switch (code & 255)
    {
    case SQLITE_BUSY: case SQLITE_LOCKED: stable = "busy"; break;
    case SQLITE_CONSTRAINT: stable = "constraint_violation"; break;
    case SQLITE_READONLY: stable = "read_only"; break;
    case SQLITE_TOOBIG: stable = "limit_exceeded"; break;
    case SQLITE_RANGE: stable = "parameter_missing"; break;
    case SQLITE_AUTH: case SQLITE_MISUSE: stable = "invalid_state"; break;
    case SQLITE_MISMATCH: stable = "type_mismatch"; break;
    case SQLITE_NOMEM: throw std::bad_alloc();
    }
    // sqlite3_errmsg 可能含 SQL 或外部数据；诊断只给稳定码和原生扩展码。
    throw runtime_failure({tx::error_kind::database, stable,
        "SQLite 操作失败（扩展码 " + std::to_string(code) + "）"});
}

void db_sqlite_check(const db_connection& connection, int status)
{
    if (status != SQLITE_OK)
    {
        db_sqlite_failure(connection, status);
    }
}

void db_finish_cursor(db_cursor_state& cursor, bool closed) noexcept
{
    if (!cursor.finished)
    {
        const auto& statement = cursor.statement;
        if (statement)
        {
            if (statement->native)
            {
                sqlite3_reset(statement->native.get());
            }
            if (cursor.pg_started && statement->connection->postgres)
            {
                if (db_current_operation)
                {
                    db_pg_cancel_pending(*statement->connection);
                }
                // 未消费结果不能返回池或交给下一条语句；不无界等待服务器。
                statement->connection->postgres.reset();
                statement->connection->cleanup_failed = true;
            }
            statement->cursor.reset();
            statement->connection->cursor.reset();
        }
        cursor.finished = true;
    }
    cursor.closed = cursor.closed || closed;
}

void db_abort_cursor(const db_connection& connection) noexcept
{
    if (const auto cursor = connection->cursor.lock())
    {
        db_finish_cursor(*cursor, true);
    }
    connection->cursor.reset();
}

db_cursor_state::~db_cursor_state()
{
    db_finish_cursor(*this, true);
}

bool db_close_cursor(const db_cursor& cursor)
{
    if (!cursor)
    {
        return false;
    }
    db_check_thread(cursor->statement->connection);
    if (!db_statement_open(*cursor->statement) ||
        !db_connection_open(*cursor->statement->connection))
    {
        cursor->closed = true;
        return false;
    }
    const bool changed = !cursor->closed;
    db_finish_cursor(*cursor, true);
    return changed;
}

bool db_close_statement(const db_statement& statement)
{
    if (!statement)
    {
        return false;
    }
    db_check_thread(statement->connection);
    if (!db_statement_open(*statement))
    {
        return false;
    }
    if (const auto cursor = statement->cursor.lock())
    {
        db_finish_cursor(*cursor, true);
    }
    statement->native.reset();
    if (statement->pg_open && statement->connection->postgres)
    {
        statement->connection->retired_statements.push_back(statement->pg_name);
    }
    statement->pg_open = false;
    statement->bound.clear();
    return true;
}

bool db_close(const db_connection& connection)
{
    db_check_thread(connection);
    if (connection->pool)
    {
        return db_release(connection);
    }
    if (!db_connection_open(*connection))
    {
        return false;
    }
    db_abort_cursor(connection);
    for (const auto& weak : connection->statements)
    {
        if (const auto statement = weak.lock())
        {
            db_close_statement(statement);
        }
    }
    // 关闭引擎也会撤销未提交事务；先显式回滚以报告真实失败。
    if (db_in_transaction(*connection))
    {
        db_control_execute(connection, "ROLLBACK");
    }
    db_reconcile_transaction(connection);
    if (connection->native)
    {
        db_sqlite_check(connection, sqlite3_close(connection->native.get()));
        connection->native.release();
    }
    connection->postgres.reset();
    connection->statements.clear();
    return true;
}

std::int64_t db_extended_error(const db_connection& connection)
{
    db_check_connection(connection);
    return connection->last_error;
}

} // namespace tx_generated
