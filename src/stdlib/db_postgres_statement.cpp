#include "stdlib/db_internal.hpp"

#include <charconv>
#include <cctype>

namespace tx_generated
{

void db_pg_validate_sql(std::string_view sql)
{
    // 只识别首个关键字；字符串和参数交给 PostgreSQL 的扩展协议解析。
    for (;;)
    {
        while (!sql.empty() && (std::isspace(static_cast<unsigned char>(sql.front())) ||
                                sql.front() == ';'))
        {
            sql.remove_prefix(1);
        }
        if (sql.starts_with("--"))
        {
            const auto end = sql.find('\n');
            sql = end == sql.npos ? std::string_view{} : sql.substr(end + 1);
            continue;
        }
        if (!sql.starts_with("/*"))
        {
            break;
        }
        std::size_t index = 2;
        int depth = 1;
        while (index < sql.size() && depth)
        {
            if (sql.substr(index).starts_with("/*"))
            {
                ++depth;
                index += 2;
            }
            else if (sql.substr(index).starts_with("*/"))
            {
                --depth;
                index += 2;
            }
            else
            {
                ++index;
            }
        }
        sql.remove_prefix(index);
    }
    std::string keyword;
    for (const auto value : sql)
    {
        if (!std::isalpha(static_cast<unsigned char>(value)))
        {
            break;
        }
        keyword.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(value))));
    }
    for (const auto* denied : {"BEGIN", "START", "COMMIT", "END", "ROLLBACK", "ABORT",
        "SAVEPOINT", "RELEASE", "PREPARE", "EXECUTE", "DEALLOCATE", "DISCARD",
        "SET", "RESET", "COPY", "CALL", "DO"})
    {
        if (keyword == denied)
        {
            db_fail("invalid_state", "该 PostgreSQL 控制语句不能通过普通查询执行");
        }
    }
    if (keyword.empty())
    {
        db_fail("invalid_argument", "SQL 中没有可执行语句");
    }
}

void db_pg_prepare(const db_statement& statement, std::string_view sql)
{
    auto& connection = *statement->connection;
    db_pg_validate_sql(sql);
    for (const auto& retired : connection.retired_statements)
    {
        db_pg_control(connection, "DEALLOCATE " + retired);
    }
    connection.retired_statements.clear();
    statement->pg_name = "tx_stmt_" + std::to_string(++connection.statement_sequence);
    db_pg_start(connection);
    const std::string text(sql);
    if (!PQsendPrepare(connection.postgres.get(), statement->pg_name.c_str(), text.c_str(), 0, nullptr))
    {
        db_pg_lost(connection);
    }
    auto prepared = db_pg_receive(connection);
    db_pg_check(connection, prepared.get());
    while (db_pg_receive(connection))
    {
    }
    statement->pg_open = true;
    if (!PQsendDescribePrepared(connection.postgres.get(), statement->pg_name.c_str()))
    {
        db_pg_lost(connection);
    }
    auto description = db_pg_receive(connection);
    db_pg_check(connection, description.get());
    const auto count = PQnparams(description.get());
    statement->pg_columns = PQnfields(description.get());
    while (db_pg_receive(connection))
    {
    }
    if (count > 4096 || statement->pg_columns > 1024)
    {
        db_fail("limit_exceeded", "PostgreSQL 参数或列数量超过上限");
    }
    statement->bound.resize(count);
}

void db_pg_validate_binding(const db_value& value)
{
    if ((value.kind == db_value_kind::text || value.kind == db_value_kind::decimal ||
         value.kind == db_value_kind::datetime) &&
        std::get<std::string>(value.data).find('\0') != std::string::npos)
    {
        db_fail("invalid_argument", "PostgreSQL 文本参数不能包含 NUL");
    }
}

namespace
{

std::string parameter_text(const db_value& value)
{
    if (value.kind == db_value_kind::integer)
    {
        return std::to_string(std::get<std::int64_t>(value.data));
    }
    if (value.kind == db_value_kind::floating)
    {
        char buffer[64];
        const auto result = std::to_chars(buffer, buffer + sizeof(buffer), std::get<double>(value.data));
        return {buffer, result.ptr};
    }
    if (value.kind == db_value_kind::boolean)
    {
        return std::get<bool>(value.data) ? "true" : "false";
    }
    if (value.kind == db_value_kind::null || value.kind == db_value_kind::binary)
    {
        return {};
    }
    return std::get<std::string>(value.data);
}

} // namespace

void db_pg_send(const db_statement& statement)
{
    auto& connection = *statement->connection;
    std::vector<std::string> storage;
    std::vector<const char*> values;
    std::vector<int> lengths, formats;
    storage.reserve(statement->bound.size());
    for (const auto& item : statement->bound)
    {
        const auto& value = *item;
        storage.push_back(parameter_text(value));
        if (value.kind == db_value_kind::binary)
        {
            const auto& bytes = std::get<byte_value>(value.data);
            values.push_back(bytes->empty() ? "" : reinterpret_cast<const char*>(bytes->data()));
            lengths.push_back(static_cast<int>(bytes->size()));
            formats.push_back(1);
        }
        else
        {
            values.push_back(value.kind == db_value_kind::null ? nullptr : storage.back().c_str());
            lengths.push_back(0);
            formats.push_back(0);
        }
    }
    db_pg_start(connection);
    if (!PQsendQueryPrepared(connection.postgres.get(), statement->pg_name.c_str(),
        static_cast<int>(values.size()), values.data(), lengths.data(), formats.data(), 0) ||
        !PQsetSingleRowMode(connection.postgres.get()))
    {
        db_pg_lost(connection);
    }
}

db_execution db_pg_execute(const db_statement& statement)
{
    if (statement->pg_columns != 0)
    {
        db_fail("invalid_state", "返回列的 SQL 必须使用 query 逐行读取");
    }
    auto& connection = *statement->connection;
    db_pg_send(statement);
    auto result = db_pg_receive(connection);
    db_pg_check(connection, result.get());
    std::int64_t affected = 0;
    const std::string_view count(PQcmdTuples(result.get()));
    if (!count.empty())
    {
        const auto parsed = std::from_chars(count.data(), count.data() + count.size(), affected);
        if (parsed.ec != std::errc{})
        {
            db_pg_lost(connection, "limit_exceeded");
        }
    }
    while (auto tail = db_pg_receive(connection))
    {
        db_pg_check(connection, tail.get());
    }
    db_reconcile_transaction(statement->connection);
    return {affected, std::nullopt};
}

} // namespace tx_generated
