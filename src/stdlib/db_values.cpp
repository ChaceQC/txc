#include "stdlib/db.hpp"
#include "common/utf8.hpp"

#include <cmath>

namespace tx_generated
{
namespace
{

bool require_kind(const db_value& value, db_value_kind kind)
{
    if (value.kind == db_value_kind::null)
    {
        return false;
    }
    if (value.kind != kind)
    {
        db_fail("type_mismatch", "数据库值与请求的类型不符");
    }
    return true;
}

} // namespace

[[noreturn]] void db_fail(const char* code, const char* message)
{
    throw runtime_failure({tx::error_kind::database, code, message});
}

std::string_view db_kind_name(const db_value& value)
{
    switch (value.kind)
    {
    case db_value_kind::null: return "null";
    case db_value_kind::integer: return "int";
    case db_value_kind::floating: return "float";
    case db_value_kind::boolean: return "bool";
    case db_value_kind::text: return "str";
    case db_value_kind::binary: return "bytes";
    case db_value_kind::decimal: return "decimal";
    case db_value_kind::datetime: return "datetime";
    }
    db_fail("invalid_state", "数据库值标记无效");
}

std::string_view db_runtime_type(const std::any& value)
{
    if (value.type() == typeid(db_pool))
    {
        return "db_pool";
    }
    if (value.type() == typeid(db_connection))
    {
        return "db_connection";
    }
    if (value.type() == typeid(db_statement))
    {
        return "db_statement";
    }
    if (value.type() == typeid(db_cursor))
    {
        return "db_cursor";
    }
    if (value.type() == typeid(db_transaction))
    {
        return "db_transaction";
    }
    if (value.type() == typeid(db_row))
    {
        return "db_row";
    }
    if (value.type() == typeid(db_value))
    {
        return "db_value";
    }
    return {};
}

bool db_type_matches(const std::any& value, std::string_view name)
{
    const auto actual = db_runtime_type(value);
    return !actual.empty() && actual == name;
}

void db_validate_value(const db_value& value, std::int64_t limit)
{
    std::size_t size = 0;
    if (value.kind == db_value_kind::floating &&
        !std::isfinite(std::get<double>(value.data)))
    {
        db_fail("invalid_argument", "SQLite 浮点参数必须为有限值");
    }
    if (const auto* text = std::get_if<std::string>(&value.data))
    {
        size = text->size();
        if (!tx::scan_utf8(*text).valid)
        {
            db_fail("invalid_encoding", "数据库文本不是有效 UTF-8");
        }
    }
    if (const auto* bytes = std::get_if<byte_value>(&value.data))
    {
        size = bytes_length(*bytes);
    }
    if (size > static_cast<std::size_t>(limit))
    {
        db_fail("limit_exceeded", "数据库值超过字节上限");
    }
}

const db_value& db_column(const db_row& row, std::int64_t index)
{
    if (!row || index < 0 || static_cast<std::size_t>(index) >= row->values.size())
    {
        db_fail("column_missing", "数据库行中不存在指定列序号");
    }
    return row->values[static_cast<std::size_t>(index)];
}

std::int64_t db_column_index(const db_row& row, std::string_view name)
{
    std::int64_t found = -1;
    if (row)
    {
        for (std::size_t index = 0; index < row->names.size(); ++index)
        {
            if (row->names[index] != name)
            {
                continue;
            }
            if (found != -1)
            {
                db_fail("ambiguous_column", "数据库行中指定列名有歧义");
            }
            found = static_cast<std::int64_t>(index);
        }
    }
    if (found == -1)
    {
        db_fail("column_missing", "数据库行中不存在指定列名");
    }
    return found;
}

std::optional<std::int64_t> db_as_int(const db_value& value)
{
    if (!require_kind(value, db_value_kind::integer))
    {
        return std::nullopt;
    }
    return std::get<std::int64_t>(value.data);
}

std::optional<double> db_as_float(const db_value& value)
{
    if (!require_kind(value, db_value_kind::floating))
    {
        return std::nullopt;
    }
    const auto result = std::get<double>(value.data);
    if (!std::isfinite(result))
    {
        db_fail("type_mismatch", "数据库浮点值不是有限数");
    }
    return result;
}

std::optional<bool> db_as_bool(const db_value& value)
{
    if (value.kind == db_value_kind::boolean)
    {
        return std::get<bool>(value.data);
    }
    if (!require_kind(value, db_value_kind::integer))
    {
        return std::nullopt;
    }
    const auto result = std::get<std::int64_t>(value.data);
    if (result != 0 && result != 1)
    {
        db_fail("type_mismatch", "数据库布尔列需要整数 0 或 1");
    }
    return result == 1;
}

std::optional<std::string> db_as_str(const db_value& value)
{
    if (!require_kind(value, db_value_kind::text))
    {
        return std::nullopt;
    }
    return std::get<std::string>(value.data);
}

std::optional<byte_value> db_as_bytes(const db_value& value)
{
    if (!require_kind(value, db_value_kind::binary))
    {
        return std::nullopt;
    }
    return std::get<byte_value>(value.data);
}

std::optional<std::string> db_domain_text(const db_value& value,
                                         db_value_kind kind)
{
    if (value.kind == db_value_kind::null)
    {
        return std::nullopt;
    }
    if (value.kind != kind && value.kind != db_value_kind::text)
    {
        db_fail("type_mismatch", "数据库领域值需要文本存储类型");
    }
    return std::get<std::string>(value.data);
}

} // namespace tx_generated
