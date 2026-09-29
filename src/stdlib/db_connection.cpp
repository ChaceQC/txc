#include "stdlib/db_internal.hpp"
#include "common/utf8.hpp"

#include <algorithm>
#include <limits>

namespace tx_generated
{
namespace
{

bool read_pragma(const char* name)
{
    if (!name)
    {
        return false;
    }
    for (const auto* allowed : {"table_info", "table_xinfo", "table_list",
        "index_list", "index_info", "index_xinfo", "foreign_key_list",
        "foreign_key_check", "integrity_check", "quick_check"})
    {
        if (sqlite3_stricmp(name, allowed) == 0)
        {
            return true;
        }
    }
    return false;
}

void observe_insert(void* user, sqlite3* native, int action,
    const char* schema, const char* table, sqlite3_int64, sqlite3_int64) noexcept
{
    auto& connection = *static_cast<db_connection_state*>(user);
    auto* insert = connection.observing_insert;
    // UPSERT 的 UPDATE 分支和触发器插入不能冒充本语句的插入标识。
    if (insert && action == SQLITE_INSERT && sqlite3_preupdate_depth(native) == 0 &&
        schema && table && insert->schema == schema && insert->table == table)
    {
        insert->observed = true;
    }
}

int authorize(void* user, int action, const char* first, const char* second,
              const char* schema, const char* source) noexcept
{
    auto& connection = *static_cast<db_connection_state*>(user);
    if (connection.control_sql)
    {
        return SQLITE_OK;
    }
    if (action == SQLITE_TRANSACTION || action == SQLITE_SAVEPOINT ||
        action == SQLITE_ATTACH || action == SQLITE_DETACH ||
        (action == SQLITE_PRAGMA && second && !read_pragma(first)))
    {
        return SQLITE_DENY;
    }
    if (action == SQLITE_INSERT && !source && connection.collecting_insert &&
        first && std::string_view(first) != "sqlite_master" &&
        std::string_view(first) != "sqlite_temp_master")
    {
        try
        {
            *connection.collecting_insert = {schema ? schema : "main", first};
        }
        catch (...)
        {
            connection.authorizer_failed = true;
            return SQLITE_DENY;
        }
    }
    return SQLITE_OK;
}

void validate_options(const db_options& options)
{
    if (options.driver != "sqlite")
    {
        db_fail("unsupported_driver", "数据库驱动尚不支持");
    }
    if (options.path.empty() || options.path.find('\0') != std::string::npos ||
        !tx::scan_utf8(options.path).valid || options.path.starts_with("file:"))
    {
        db_fail("invalid_argument", "SQLite 需要有效 UTF-8 文件路径，不能使用 URI");
    }
    if (options.busy_timeout_ms < 0 ||
        options.busy_timeout_ms > std::numeric_limits<int>::max() ||
        options.max_value_bytes < 1 || options.max_value_bytes > 67108864)
    {
        db_fail("invalid_argument", "SQLite 忙等待或字节上限超出范围");
    }
    if (options.wal && (options.read_only || options.path == ":memory:"))
    {
        db_fail("invalid_argument", "SQLite WAL 需要可写文件数据库");
    }
}

void enable_wal(const db_connection& connection)
{
    db_control_guard guard(*connection);
    sqlite3_stmt* raw = nullptr;
    const auto status = sqlite3_prepare_v2(connection->native.get(),
        "PRAGMA journal_mode=WAL", -1, &raw, nullptr);
    std::unique_ptr<sqlite3_stmt, db_statement_deleter> statement(raw);
    db_sqlite_check(connection, status);
    const auto step = sqlite3_step(statement.get());
    if (step != SQLITE_ROW)
    {
        db_sqlite_failure(connection, step);
    }
    const auto* text = sqlite3_column_text(statement.get(), 0);
    if (!text || std::string_view(reinterpret_cast<const char*>(text)) != "wal")
    {
        db_fail("connection_failed", "SQLite 未能启用 WAL");
    }
}

} // namespace

db_connection db_open(const db_options& options)
{
    validate_options(options);
    auto connection = std::make_shared<db_connection_state>();
    connection->value_limit = options.max_value_bytes;
    const int flags = (options.read_only ? SQLITE_OPEN_READONLY :
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE) | SQLITE_OPEN_FULLMUTEX;
    sqlite3* raw = nullptr;
    const auto status = sqlite3_open_v2(options.path.c_str(), &raw, flags, nullptr);
    connection->native.reset(raw);
    if (status != SQLITE_OK)
    {
        db_sqlite_failure(connection, status, true);
    }
    db_sqlite_check(connection, sqlite3_extended_result_codes(raw, 1));
    db_sqlite_check(connection,
        sqlite3_busy_timeout(raw, static_cast<int>(options.busy_timeout_ms)));
    sqlite3_limit(raw, SQLITE_LIMIT_LENGTH, static_cast<int>(options.max_value_bytes));
    sqlite3_limit(raw, SQLITE_LIMIT_SQL_LENGTH, 1048576);
    sqlite3_limit(raw, SQLITE_LIMIT_VARIABLE_NUMBER, 4096);
    sqlite3_limit(raw, SQLITE_LIMIT_COLUMN, 1024);
    db_sqlite_check(connection, sqlite3_db_config(raw, SQLITE_DBCONFIG_ENABLE_FKEY, 1, nullptr));
    db_sqlite_check(connection, sqlite3_db_config(raw, SQLITE_DBCONFIG_TRUSTED_SCHEMA, 0, nullptr));
    db_sqlite_check(connection, sqlite3_db_config(raw, SQLITE_DBCONFIG_DEFENSIVE, 1, nullptr));
    db_sqlite_check(connection, sqlite3_set_authorizer(raw, authorize, connection.get()));
    sqlite3_preupdate_hook(raw, observe_insert, connection.get());
    if (options.wal)
    {
        enable_wal(connection);
    }
    return connection;
}

db_statement db_prepare(const db_connection& connection, std::string_view sql)
{
    db_check_connection(connection, true);
    if (sql.empty() || sql.find('\0') != std::string_view::npos ||
        !tx::scan_utf8(sql).valid)
    {
        db_fail("invalid_argument", "SQL 必须是非空有效 UTF-8，不能包含 NUL");
    }
    if (sql.size() > 1048576)
    {
        db_fail("limit_exceeded", "SQL 超过 1 MiB 上限");
    }
    std::erase_if(connection->statements, [](const auto& item)
    {
        const auto value = item.lock();
        return !value || !db_statement_open(*value);
    });
    if (connection->statements.size() >= 4096)
    {
        db_fail("limit_exceeded", "数据库连接的语句数量超过上限");
    }
    auto statement = std::make_shared<db_statement_state>();
    statement->connection = connection;
    if (connection->pg_driver)
    {
        db_pg_prepare(statement, sql);
        connection->statements.push_back(statement);
        return statement;
    }
    const std::string text(sql);
    const char* tail = nullptr;
    sqlite3_stmt* raw = nullptr;
    connection->collecting_insert = &statement->insert;
    connection->authorizer_failed = false;
    const auto status = sqlite3_prepare_v3(connection->native.get(), text.c_str(),
        static_cast<int>(text.size()), SQLITE_PREPARE_PERSISTENT, &raw, &tail);
    connection->collecting_insert = nullptr;
    statement->native.reset(raw);
    if (connection->authorizer_failed)
    {
        throw std::bad_alloc();
    }
    db_sqlite_check(connection, status);
    if (!raw)
    {
        db_fail("invalid_argument", "SQL 中没有可执行语句");
    }
    // 让 SQLite 识别尾部注释；任何第二条实际语句都被拒绝且不执行。
    while (tail && *tail)
    {
        sqlite3_stmt* extra_raw = nullptr;
        const char* next = nullptr;
        const auto extra_status = sqlite3_prepare_v2(connection->native.get(),
            tail, -1, &extra_raw, &next);
        std::unique_ptr<sqlite3_stmt, db_statement_deleter> extra(extra_raw);
        db_sqlite_check(connection, extra_status);
        if (extra)
        {
            db_fail("invalid_argument", "一次只能准备一条 SQL 语句");
        }
        tail = next;
    }
    statement->bound.resize(sqlite3_bind_parameter_count(raw));
    connection->statements.push_back(statement);
    return statement;
}

} // namespace tx_generated
