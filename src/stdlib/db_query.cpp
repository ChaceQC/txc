#include "stdlib/db_internal.hpp"
#include "common/utf8.hpp"

#include <cmath>
#include <cstring>

namespace tx_generated
{
namespace
{

std::string column_text(sqlite3_stmt* statement, int index)
{
    const auto* data = sqlite3_column_text(statement, index);
    const auto size = sqlite3_column_bytes(statement, index);
    if (!data)
    {
        throw std::bad_alloc();
    }
    std::string result(reinterpret_cast<const char*>(data),
                       static_cast<std::size_t>(size));
    if (!tx::scan_utf8(result).valid)
    {
        db_fail("invalid_encoding", "数据库 TEXT 不是有效 UTF-8");
    }
    return result;
}

db_value read_column(sqlite3_stmt* statement, int index, std::size_t& bytes)
{
    switch (sqlite3_column_type(statement, index))
    {
    case SQLITE_NULL:
        return {};
    case SQLITE_INTEGER:
        return {db_value_kind::integer, std::int64_t(sqlite3_column_int64(statement, index))};
    case SQLITE_FLOAT:
    {
        const auto number = sqlite3_column_double(statement, index);
        if (!std::isfinite(number))
        {
            db_fail("type_mismatch", "数据库浮点值不是有限数");
        }
        return {db_value_kind::floating, number};
    }
    case SQLITE_TEXT:
    {
        auto text = column_text(statement, index);
        bytes += text.size();
        return {db_value_kind::text, std::move(text)};
    }
    case SQLITE_BLOB:
    {
        const auto* data = static_cast<const std::uint8_t*>(sqlite3_column_blob(statement, index));
        const auto size = sqlite3_column_bytes(statement, index);
        if (!data && size != 0)
        {
            throw std::bad_alloc();
        }
        std::vector<std::uint8_t> buffer;
        if (size != 0)
        {
            buffer.assign(data, data + size);
        }
        bytes += buffer.size();
        return {db_value_kind::binary, make_bytes(std::move(buffer))};
    }
    }
    db_fail("type_mismatch", "SQLite 列的存储类型无效");
}

db_row snapshot(const db_statement& statement)
{
    auto row = std::make_shared<db_row_data>();
    const auto count = sqlite3_column_count(statement->native.get());
    row->names.reserve(count);
    row->values.reserve(count);
    std::size_t bytes = 0;
    for (int index = 0; index < count; ++index)
    {
        const auto* name = sqlite3_column_name(statement->native.get(), index);
        if (!name)
        {
            throw std::bad_alloc();
        }
        std::string text(name);
        if (!tx::scan_utf8(text).valid)
        {
            db_fail("invalid_encoding", "数据库列名不是有效 UTF-8");
        }
        bytes += text.size();
        row->names.push_back(std::move(text));
        row->values.push_back(read_column(statement->native.get(), index, bytes));
        if (bytes > static_cast<std::size_t>(statement->connection->value_limit))
        {
            db_fail("limit_exceeded", "数据库行快照超过字节上限");
        }
    }
    return row;
}

} // namespace

std::optional<std::int64_t> db_insert_id(const db_statement& statement)
{
    if (statement->insert.table.empty() || !statement->insert.observed)
    {
        return std::nullopt;
    }
    // WITHOUT ROWID 不更新 last_insert_rowid；先看实际表能力，避免返回旧标识。
    sqlite3_stmt* raw = nullptr;
    const auto& connection = statement->connection;
    const auto status = sqlite3_prepare_v2(connection->native.get(),
        "SELECT wr FROM pragma_table_list WHERE schema=? AND name=?", -1, &raw, nullptr);
    std::unique_ptr<sqlite3_stmt, db_statement_deleter> metadata(raw);
    db_sqlite_check(connection, status);
    db_sqlite_check(connection, sqlite3_bind_text(raw, 1,
        statement->insert.schema.c_str(), -1, SQLITE_TRANSIENT));
    db_sqlite_check(connection, sqlite3_bind_text(raw, 2,
        statement->insert.table.c_str(), -1, SQLITE_TRANSIENT));
    const auto step = sqlite3_step(raw);
    if (step == SQLITE_DONE || (step == SQLITE_ROW && sqlite3_column_int(raw, 0) != 0))
    {
        return std::nullopt;
    }
    if (step != SQLITE_ROW)
    {
        db_sqlite_failure(connection, step);
    }
    return sqlite3_last_insert_rowid(connection->native.get());
}

db_execution db_execute(const db_statement& statement)
{
    db_check_statement(statement);
    db_check_bound(statement);
    if (statement->connection->pg_driver)
    {
        return db_pg_execute(statement);
    }
    if (sqlite3_column_count(statement->native.get()) != 0)
    {
        db_fail("invalid_state", "返回列的 SQL 必须使用 query 逐行读取");
    }
    auto* native = statement->connection->native.get();
    const auto previous = sqlite3_total_changes64(native);
    statement->insert.observed = false;
    statement->connection->observing_insert = &statement->insert;
    const auto step = sqlite3_step(statement->native.get());
    statement->connection->observing_insert = nullptr;
    const auto changed = sqlite3_total_changes64(native) != previous;
    const auto affected = changed ? sqlite3_changes64(native) : 0;
    sqlite3_reset(statement->native.get());
    if (step != SQLITE_DONE)
    {
        db_sqlite_failure(statement->connection, step);
    }
    db_reconcile_transaction(statement->connection);
    db_execution result{affected, std::nullopt};
    if (affected != 0)
    {
        result.insert_id = db_insert_id(statement);
    }
    return result;
}

db_cursor db_query(const db_statement& statement)
{
    db_check_statement(statement);
    db_check_bound(statement);
    if ((statement->connection->pg_driver ? statement->pg_columns :
        sqlite3_column_count(statement->native.get())) == 0)
    {
        db_fail("invalid_state", "没有返回列的 SQL 应使用 execute");
    }
    auto cursor = std::make_shared<db_cursor_state>();
    cursor->statement = statement;
    statement->cursor = cursor;
    statement->connection->cursor = cursor;
    return cursor;
}

db_row db_next(const db_cursor& cursor)
{
    if (!cursor || cursor->closed)
    {
        db_fail("invalid_state", "数据库游标已关闭");
    }
    db_check_statement(cursor->statement, false);
    if (cursor->finished)
    {
        return {};
    }
    const auto& statement = cursor->statement;
    try
    {
        if (statement->connection->pg_driver)
        {
            return db_pg_next(cursor);
        }
        const auto step = sqlite3_step(statement->native.get());
        if (step == SQLITE_DONE)
        {
            db_finish_cursor(*cursor, false);
            db_reconcile_transaction(statement->connection);
            return {};
        }
        if (step != SQLITE_ROW)
        {
            db_sqlite_failure(statement->connection, step);
        }
        return snapshot(statement);
    }
    catch (...)
    {
        db_finish_cursor(*cursor, true);
        db_reconcile_transaction(statement->connection);
        throw;
    }
}

} // namespace tx_generated
