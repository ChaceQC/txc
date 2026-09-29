#include "stdlib/db_internal.hpp"
#include <charconv>

namespace tx_generated
{
namespace
{

int bind_value(sqlite3_stmt* statement, int index, const db_value& value)
{
    switch (value.kind)
    {
    case db_value_kind::null:
        return sqlite3_bind_null(statement, index);
    case db_value_kind::integer:
        return sqlite3_bind_int64(statement, index, std::get<std::int64_t>(value.data));
    case db_value_kind::floating:
        return sqlite3_bind_double(statement, index, std::get<double>(value.data));
    case db_value_kind::boolean:
        return sqlite3_bind_int(statement, index, std::get<bool>(value.data) ? 1 : 0);
    case db_value_kind::text: case db_value_kind::decimal: case db_value_kind::datetime:
    {
        const auto& text = std::get<std::string>(value.data);
        return sqlite3_bind_text64(statement, index, text.data(), text.size(),
                                   SQLITE_TRANSIENT, SQLITE_UTF8);
    }
    case db_value_kind::binary:
    {
        const auto& bytes = std::get<byte_value>(value.data);
        return sqlite3_bind_blob64(statement, index, bytes->data(), bytes->size(),
                                   SQLITE_TRANSIENT);
    }
    }
    return SQLITE_MISMATCH;
}

int replace_bindings(const db_statement& statement,
                      const std::vector<std::optional<db_value>>& values)
{
    const auto cleared = sqlite3_clear_bindings(statement->native.get());
    if (cleared != SQLITE_OK)
    {
        return cleared;
    }
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (!values[index])
        {
            continue;
        }
        const auto status = bind_value(statement->native.get(),
            static_cast<int>(index + 1), *values[index]);
        if (status != SQLITE_OK)
        {
            return status;
        }
    }
    return SQLITE_OK;
}

} // namespace

std::int64_t db_parameter_count(const db_statement& statement)
{
    db_check_statement(statement, false);
    return static_cast<std::int64_t>(statement->bound.size());
}

std::int64_t db_parameter_index(const db_statement& statement, std::string_view name)
{
    db_check_statement(statement, false);
    if (name.find('\0') != std::string_view::npos)
    {
        db_fail("invalid_argument", "参数名称不能包含 NUL");
    }
    const std::string text(name);
    if (statement->connection->pg_driver)
    {
        std::int64_t index = 0;
        const auto parsed = name.starts_with('$')
            ? std::from_chars(name.data() + 1, name.data() + name.size(), index)
            : std::from_chars_result{name.data(), std::errc::invalid_argument};
        if (parsed.ec != std::errc{} || parsed.ptr != name.data() + name.size() ||
            index < 1 || index > static_cast<std::int64_t>(statement->bound.size()))
        {
            db_fail("parameter_missing", "PostgreSQL 参数名称必须是存在的 $序号");
        }
        return index;
    }
    const auto index = sqlite3_bind_parameter_index(statement->native.get(), text.c_str());
    if (index == 0)
    {
        db_fail("parameter_missing", "SQL 中不存在指定参数名称");
    }
    return index;
}

void db_bind(const db_statement& statement, std::int64_t index, const db_value& value)
{
    db_check_statement(statement);
    if (index < 1 || static_cast<std::size_t>(index) > statement->bound.size())
    {
        db_fail("parameter_missing", "数据库参数序号从 1 开始且不能超过参数数量");
    }
    db_validate_value(value, statement->connection->value_limit);
    auto copied = value;
    if (statement->connection->pg_driver)
    {
        db_pg_validate_binding(value);
        statement->bound[static_cast<std::size_t>(index - 1)] = std::move(copied);
        return;
    }
    const auto status = bind_value(statement->native.get(), static_cast<int>(index), copied);
    if (status != SQLITE_OK)
    {
        // 原生绑定失败可能清除目标槽位，恢复旧绑定后才允许继续使用。
        if (replace_bindings(statement, statement->bound) != SQLITE_OK)
        {
            db_close_statement(statement);
        }
        db_sqlite_failure(statement->connection, status);
    }
    statement->bound[static_cast<std::size_t>(index - 1)] = std::move(copied);
}

void db_bind_all(const db_statement& statement, const std::vector<db_value>& values)
{
    db_check_statement(statement);
    if (values.size() != statement->bound.size())
    {
        db_fail("parameter_missing", "批量绑定的参数数量必须与 SQL 完全相同");
    }
    std::vector<std::optional<db_value>> rebuilt;
    rebuilt.reserve(values.size());
    for (const auto& value : values)
    {
        db_validate_value(value, statement->connection->value_limit);
        if (statement->connection->pg_driver)
        {
            db_pg_validate_binding(value);
        }
        rebuilt.emplace_back(value);
    }
    const auto status = statement->connection->pg_driver
        ? SQLITE_OK : replace_bindings(statement, rebuilt);
    if (status != SQLITE_OK)
    {
        if (replace_bindings(statement, statement->bound) != SQLITE_OK)
        {
            db_close_statement(statement);
        }
        db_sqlite_failure(statement->connection, status);
    }
    statement->bound.swap(rebuilt);
}

void db_clear_bindings(const db_statement& statement)
{
    db_check_statement(statement);
    if (statement->native)
    {
        db_sqlite_check(statement->connection, sqlite3_clear_bindings(statement->native.get()));
    }
    for (auto& value : statement->bound)
    {
        value.reset();
    }
}

} // namespace tx_generated
