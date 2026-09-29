#include "stdlib/db_internal.hpp"
#include "common/utf8.hpp"

#include <charconv>
#include <cmath>

namespace tx_generated
{
namespace
{

template<class number_type>
number_type parse_number(std::string_view text)
{
    number_type value{};
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
    {
        db_fail("type_mismatch", "PostgreSQL 数值不能表示为请求的 TX 类型");
    }
    return value;
}

db_value column_value(PGresult* result, int index)
{
    if (PQgetisnull(result, 0, index))
    {
        return {};
    }
    const std::string_view text(PQgetvalue(result, 0, index), PQgetlength(result, 0, index));
    switch (PQftype(result, index))
    {
    case 16:
        if (text != "t" && text != "f")
        {
            db_fail("type_mismatch", "PostgreSQL 布尔值不合法");
        }
        return {db_value_kind::boolean, text == "t"};
    case 20: case 21: case 23:
        return {db_value_kind::integer, parse_number<std::int64_t>(text)};
    case 700: case 701:
    {
        const auto value = parse_number<double>(text);
        if (!std::isfinite(value))
        {
            db_fail("type_mismatch", "PostgreSQL 非有限浮点值不受支持");
        }
        return {db_value_kind::floating, value};
    }
    case 17:
    {
        std::size_t length = 0;
        std::unique_ptr<unsigned char, decltype(&PQfreemem)> data(
            PQunescapeBytea(reinterpret_cast<const unsigned char*>(text.data()), &length), PQfreemem);
        if (!data)
        {
            throw std::bad_alloc();
        }
        return {db_value_kind::binary, make_bytes({data.get(), data.get() + length})};
    }
    case 1700:
        if (text == "NaN" || text == "Infinity" || text == "-Infinity")
        {
            db_fail("type_mismatch", "PostgreSQL 非有限 decimal 不受支持");
        }
        return {db_value_kind::decimal, std::string(text)};
    case 1184:
    {
        if (text.size() < 22 || text[10] != ' ' || !text.ends_with("+00"))
        {
            db_fail("type_mismatch", "PostgreSQL 时间超出 TX 支持的范围或不是 UTC");
        }
        std::string converted(text);
        converted[10] = 'T';
        converted += ":00";
        return {db_value_kind::datetime, std::move(converted)};
    }
    case 19: case 25: case 1042: case 1043:
        if (!tx::scan_utf8(text).valid)
        {
            db_fail("invalid_encoding", "PostgreSQL 文本不是有效 UTF-8");
        }
        return {db_value_kind::text, std::string(text)};
    default:
        db_fail("type_mismatch", "PostgreSQL 列类型不受支持，请在 SQL 中显式转换");
    }
}

db_row snapshot(PGresult* result, std::int64_t limit)
{
    const auto count = PQnfields(result);
    if (count > 1024)
    {
        db_fail("limit_exceeded", "PostgreSQL 列数量超过上限");
    }
    auto row = std::make_shared<db_row_data>();
    std::size_t size = 0;
    for (int index = 0; index < count; ++index)
    {
        const std::string name(PQfname(result, index));
        if (!tx::scan_utf8(name).valid)
        {
            db_fail("invalid_encoding", "PostgreSQL 列名不是有效 UTF-8");
        }
        size += name.size() + static_cast<std::size_t>(PQgetlength(result, 0, index));
        if (size > static_cast<std::size_t>(limit))
        {
            db_fail("limit_exceeded", "PostgreSQL 行快照超过字节上限");
        }
        row->names.push_back(name);
        row->values.push_back(column_value(result, index));
    }
    return row;
}

} // namespace

db_row db_pg_next(const db_cursor& cursor)
{
    const auto& statement = cursor->statement;
    auto& connection = *statement->connection;
    if (!cursor->pg_started)
    {
        db_pg_send(statement);
        cursor->pg_started = true;
    }
    auto result = db_pg_receive(connection);
    // 服务器错误已被消费到 ReadyForQuery，保留可回滚的失败事务。
    try
    {
        db_pg_check(connection, result.get());
    }
    catch (...)
    {
        cursor->pg_started = false;
        throw;
    }
    if (PQresultStatus(result.get()) == PGRES_SINGLE_TUPLE)
    {
        return snapshot(result.get(), connection.value_limit);
    }
    while (auto tail = db_pg_receive(connection))
    {
        db_pg_check(connection, tail.get());
    }
    cursor->pg_started = false;
    db_finish_cursor(*cursor, false);
    db_reconcile_transaction(statement->connection);
    return {};
}

} // namespace tx_generated
