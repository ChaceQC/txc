#include "backend/cpp/db_abi.hpp"
#include "backend/cpp/db_abi_internal.hpp"

using namespace tx_generated;
using namespace tx_generated::db_abi;

namespace
{

std::vector<std::string> sql_list(const void* statements)
{
    const auto& values = input<string_vector>(statements).data().values;
    if (values.size() > 4096)
    {
        db_fail("invalid_argument", "迁移 SQL 条数超过上限");
    }
    std::vector<std::string> result;
    result.reserve(values.size());
    for (const auto& value : values)
    {
        result.emplace_back(value.get());
    }
    return result;
}

} // namespace

extern "C" int txrt_db_migration_checksum(const void* statements, void** result) noexcept
{
    return detail::invoke_checked([&]
    {
        *result = detail::make_handle<std::string>(db_migration_checksum(sql_list(statements)));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_migrate(const void* connection, std::int64_t version,
    const void* statements, const void* checksum, bool* result) noexcept
{
    return detail::invoke_checked([&]
    {
        *result = db_migrate(input<db_connection>(connection), version,
            sql_list(statements), detail::text_value(checksum));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_schema_version(const void* connection, std::int64_t* result) noexcept
{
    return detail::invoke_checked([&]
    {
        *result = db_schema_version(input<db_connection>(connection));
    }, tx::error_kind::database);
}
