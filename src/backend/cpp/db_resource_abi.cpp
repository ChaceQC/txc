#include "backend/cpp/db_abi.hpp"
#include "backend/cpp/db_abi_internal.hpp"

using namespace tx_generated;
using namespace tx_generated::db_abi;
using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

extern "C" int txrt_db_close(const void* connection, bool* result) noexcept
{
    return invoke_checked([&]
    {
        *result = db_close(input<db_connection>(connection));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_extended_error_code(const void* connection, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = db_extended_error(input<db_connection>(connection));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_prepare(const void* connection, const void* sql, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(db_prepare(input<db_connection>(connection),
            detail::text_value(sql)));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_parameter_count(const void* statement, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = db_parameter_count(input<db_statement>(statement));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_parameter_index(const void* statement, const void* name, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = db_parameter_index(input<db_statement>(statement), detail::text_value(name));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_bind(const void* statement, std::int64_t index, const void* value) noexcept
{
    return invoke_checked([&]
    {
        db_bind(input<db_statement>(statement), index, input<db_value>(value));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_clear_bindings(const void* statement) noexcept
{
    return invoke_checked([&]
    {
        db_clear_bindings(input<db_statement>(statement));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_query(const void* statement, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(db_query(input<db_statement>(statement)));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_close_statement(const void* statement, bool* result) noexcept
{
    return invoke_checked([&]
    {
        *result = db_close_statement(input<db_statement>(statement));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_close_cursor(const void* cursor, bool* result) noexcept
{
    return invoke_checked([&]
    {
        *result = db_close_cursor(input<db_cursor>(cursor));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_begin(const void* connection, const void* mode, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(db_begin(input<db_connection>(connection),
            detail::text_value(mode)));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_commit(const void* transaction) noexcept
{
    return invoke_checked([&]
    {
        db_commit(input<db_transaction>(transaction));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_rollback(const void* transaction, bool* result) noexcept
{
    return invoke_checked([&]
    {
        *result = db_rollback(input<db_transaction>(transaction));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_savepoint(const void* transaction, const void* name) noexcept
{
    return invoke_checked([&]
    {
        db_savepoint(input<db_transaction>(transaction), detail::text_value(name));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_rollback_to(const void* transaction, const void* name) noexcept
{
    return invoke_checked([&]
    {
        db_rollback_to(input<db_transaction>(transaction), detail::text_value(name));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_release_savepoint(const void* transaction, const void* name) noexcept
{
    return invoke_checked([&]
    {
        db_release_savepoint(input<db_transaction>(transaction), detail::text_value(name));
    }, tx::error_kind::database);
}
