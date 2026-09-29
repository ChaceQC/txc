#pragma once

#include "stdlib/db.hpp"
#include <sqlite3.h>
#include <libpq-fe.h>

#include <thread>
#include <chrono>

namespace tx_generated
{

struct db_connection_deleter
{
    void operator()(sqlite3* value) const noexcept;
};

struct db_statement_deleter
{
    void operator()(sqlite3_stmt* value) const noexcept;
};

struct db_pg_deleter
{
    void operator()(PGconn* value) const noexcept;
};

struct db_pg_result_deleter
{
    void operator()(PGresult* value) const noexcept;
};

using db_pg_result = std::unique_ptr<PGresult, db_pg_result_deleter>;

struct db_insert_info
{
    std::string schema;
    std::string table;
    bool observed = false;
};

struct db_connection_state
{
    ~db_connection_state();
    std::unique_ptr<sqlite3, db_connection_deleter> native;
    std::unique_ptr<PGconn, db_pg_deleter> postgres;
    db_pool pool;
    bool pg_driver = false;
    bool pg_read_only = false;
    std::int64_t operation_timeout_ms = 30000;
    std::chrono::steady_clock::time_point operation_deadline;
    std::optional<std::chrono::steady_clock::time_point> connect_deadline;
    std::uint64_t statement_sequence = 0;
    std::string last_sqlstate;
    std::vector<std::string> retired_statements;
    std::thread::id owner = std::this_thread::get_id();
    std::vector<std::weak_ptr<db_statement_state>> statements;
    std::weak_ptr<db_cursor_state> cursor;
    std::weak_ptr<db_transaction_state> transaction;
    db_insert_info* collecting_insert = nullptr;
    db_insert_info* observing_insert = nullptr;
    std::int64_t value_limit = 1048576;
    int last_error = SQLITE_OK;
    bool control_sql = false;
    bool cleanup_failed = false;
    bool authorizer_failed = false;
};

struct db_statement_state
{
    ~db_statement_state();
    db_connection connection;
    std::unique_ptr<sqlite3_stmt, db_statement_deleter> native;
    std::vector<std::optional<db_value>> bound;
    std::weak_ptr<db_cursor_state> cursor;
    db_insert_info insert;
    std::string pg_name;
    int pg_columns = 0;
    bool pg_open = false;
};

struct db_cursor_state
{
    ~db_cursor_state();
    db_statement statement;
    bool finished = false;
    bool closed = false;
    bool pg_started = false;
};

struct db_transaction_state
{
    ~db_transaction_state();
    db_connection connection;
    std::vector<std::string> savepoints;
    bool active = true;
};

// 只有库内的事务和选项设置能临时通过 SQL 授权器。
struct db_control_guard
{
    explicit db_control_guard(db_connection_state& connection)
        : state(connection), previous(connection.control_sql)
    {
        state.control_sql = true;
    }
    ~db_control_guard()
    {
        state.control_sql = previous;
    }
    db_connection_state& state;
    bool previous;
};

void db_check_thread(const db_connection& connection);
void db_check_connection(const db_connection& connection, bool idle = false);
void db_check_statement(const db_statement& statement, bool idle = true);
void db_check_bound(const db_statement& statement);
void db_reconcile_transaction(const db_connection& connection);
[[noreturn]] void db_sqlite_failure(const db_connection& connection,
                                    int status, bool opening = false);
void db_sqlite_check(const db_connection& connection, int status);
void db_finish_cursor(db_cursor_state& cursor, bool closed) noexcept;
void db_abort_cursor(const db_connection& connection) noexcept;
void db_control_execute(const db_connection& connection, const std::string& sql);
std::optional<std::int64_t> db_insert_id(const db_statement& statement);
bool db_connection_open(const db_connection_state& connection) noexcept;
bool db_statement_open(const db_statement_state& statement) noexcept;
bool db_in_transaction(const db_connection_state& connection) noexcept;
void db_pg_control(db_connection_state& connection, const std::string& sql);
void db_pg_configure(db_connection_state& connection);
void db_pg_start(db_connection_state& connection);
void db_pg_cancel_pending(db_connection_state& connection) noexcept;
void db_pg_check_operation(db_connection_state& connection);
db_pg_result db_pg_receive(db_connection_state& connection);
void db_pg_check(db_connection_state& connection, PGresult* result,
                 bool opening = false);
[[noreturn]] void db_pg_lost(db_connection_state& connection,
                            const char* code = "connection_lost");
void db_pg_prepare(const db_statement& statement, std::string_view sql);
db_execution db_pg_execute(const db_statement& statement);
db_row db_pg_next(const db_cursor& cursor);
void db_pg_send(const db_statement& statement);
void db_pg_validate_sql(std::string_view sql);
void db_pg_validate_options(const db_postgres_options& options);
void db_pg_validate_binding(const db_value& value);
void db_return_lease(db_connection_state& connection) noexcept;
void db_install_sqlite_operation(const db_connection& connection, std::int64_t busy_timeout_ms);

} // namespace tx_generated
