#include "stdlib/db_internal.hpp"

#include <algorithm>

namespace tx_generated
{
namespace
{

void require_active(const db_transaction& transaction, bool idle)
{
    if (!transaction)
    {
        db_fail("invalid_state", "数据库事务句柄无效");
    }
    db_check_connection(transaction->connection, idle);
    db_reconcile_transaction(transaction->connection);
    if (!transaction->active)
    {
        db_fail("invalid_state", "数据库事务已结束");
    }
}

void validate_name(std::string_view name)
{
    if (name.empty() || name.size() > 64 ||
        !std::all_of(name.begin(), name.end(), [](unsigned char value)
        {
            return (value >= 'a' && value <= 'z') ||
                   (value >= 'A' && value <= 'Z') ||
                   (value >= '0' && value <= '9') || value == '_';
        }))
    {
        db_fail("invalid_argument", "保存点名只允许 1～64 个 ASCII 字母、数字或下划线");
    }
}

std::size_t find_savepoint(const db_transaction& transaction, std::string_view name)
{
    validate_name(name);
    const auto& names = transaction->savepoints;
    const auto found = std::find(names.begin(), names.end(), name);
    if (found == names.end())
    {
        db_fail("invalid_state", "数据库保存点不存在或已释放");
    }
    return static_cast<std::size_t>(found - names.begin());
}

} // namespace

void db_control_execute(const db_connection& connection, const std::string& sql)
{
    if (connection->pg_driver)
    {
        try
        {
            db_pg_control(*connection, sql);
        }
        catch (...)
        {
            db_reconcile_transaction(connection);
            throw;
        }
        return;
    }
    db_control_guard guard(*connection);
    const auto status = sqlite3_exec(connection->native.get(), sql.c_str(), nullptr, nullptr, nullptr);
    db_sqlite_check(connection, status);
}

db_transaction_state::~db_transaction_state()
{
    if (active && db_in_transaction(*connection))
    {
        db_abort_cursor(connection);
        if (connection->pg_driver)
        {
            if (connection->postgres)
            {
                try
                {
                    db_pg_control(*connection, "ROLLBACK");
                }
                catch (...)
                {
                    connection->postgres.reset();
                    connection->cleanup_failed = true;
                }
            }
            return;
        }
        db_control_guard guard(*connection);
        const auto status = sqlite3_exec(connection->native.get(), "ROLLBACK", nullptr, nullptr, nullptr);
        if (status != SQLITE_OK)
        {
            connection->last_error = status;
            connection->cleanup_failed = true;
        }
    }
}

db_transaction db_begin(const db_connection& connection, std::string_view mode)
{
    db_check_connection(connection, true);
    if (db_in_transaction(*connection))
    {
        db_fail("invalid_state", "连接已有事务，嵌套操作请使用保存点");
    }
    std::string command;
    if (connection->pg_driver)
    {
        if (mode == "read_committed")
        {
            command = "BEGIN ISOLATION LEVEL READ COMMITTED";
        }
        else if (mode == "repeatable_read")
        {
            command = "BEGIN ISOLATION LEVEL REPEATABLE READ";
        }
        else if (mode == "serializable")
        {
            command = "BEGIN ISOLATION LEVEL SERIALIZABLE";
        }
        else
        {
            db_fail("invalid_argument", "PostgreSQL 事务隔离级别无效");
        }
    }
    else if (mode != "deferred" && mode != "immediate" && mode != "exclusive")
    {
        db_fail("invalid_argument", "SQLite 事务模式必须为 deferred、immediate 或 exclusive");
    }
    auto transaction = std::make_shared<db_transaction_state>();
    transaction->connection = connection;
    // 创建失败的事务对象不得回滚连接上已有状态。
    transaction->active = false;
    db_control_execute(connection, command.empty() ? "BEGIN " + std::string(mode) : command);
    transaction->active = true;
    connection->transaction = transaction;
    return transaction;
}

void db_commit(const db_transaction& transaction)
{
    require_active(transaction, true);
    db_control_execute(transaction->connection, "COMMIT");
    transaction->active = false;
    transaction->savepoints.clear();
    transaction->connection->transaction.reset();
}

bool db_rollback(const db_transaction& transaction)
{
    if (!transaction)
    {
        return false;
    }
    db_check_thread(transaction->connection);
    if (!transaction->active || !db_connection_open(*transaction->connection))
    {
        return false;
    }
    require_active(transaction, false);
    db_abort_cursor(transaction->connection);
    if (!db_connection_open(*transaction->connection))
    {
        transaction->active = false;
        return false;
    }
    db_control_execute(transaction->connection, "ROLLBACK");
    transaction->active = false;
    transaction->savepoints.clear();
    transaction->connection->transaction.reset();
    return true;
}

void db_savepoint(const db_transaction& transaction, std::string_view name)
{
    require_active(transaction, true);
    validate_name(name);
    if (std::find(transaction->savepoints.begin(), transaction->savepoints.end(), name) !=
        transaction->savepoints.end())
    {
        db_fail("invalid_state", "数据库保存点名已存在");
    }
    auto rebuilt = transaction->savepoints;
    rebuilt.emplace_back(name);
    db_control_execute(transaction->connection, "SAVEPOINT \"" + std::string(name) + "\"");
    transaction->savepoints.swap(rebuilt);
}

void db_rollback_to(const db_transaction& transaction, std::string_view name)
{
    require_active(transaction, false);
    const auto index = find_savepoint(transaction, name);
    db_abort_cursor(transaction->connection);
    db_check_connection(transaction->connection);
    db_control_execute(transaction->connection, "ROLLBACK TO \"" + std::string(name) + "\"");
    transaction->savepoints.resize(index + 1);
}

void db_release_savepoint(const db_transaction& transaction, std::string_view name)
{
    require_active(transaction, true);
    const auto index = find_savepoint(transaction, name);
    db_control_execute(transaction->connection, "RELEASE \"" + std::string(name) + "\"");
    transaction->savepoints.resize(index);
}

} // namespace tx_generated
