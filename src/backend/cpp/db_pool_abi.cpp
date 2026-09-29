#include "backend/cpp/db_abi.hpp"
#include "backend/cpp/db_abi_internal.hpp"
#include <libpq-fe.h>

using namespace tx_generated;
using namespace tx_generated::db_abi;
using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

namespace
{

db_postgres_options postgres_config(const void* config)
{
    const auto& value = input<dynamic_struct>(config);
    return {field<std::string>(value, 0), field<std::int64_t>(value, 1),
        field<std::string>(value, 2), field<std::string>(value, 3),
        field<std::string>(value, 4), field<std::int64_t>(value, 5),
        field<std::int64_t>(value, 6), field<std::int64_t>(value, 7), field<bool>(value, 8)};
}

} // namespace

extern "C" int txrt_db_postgres_options(const void* host, const void* database,
    const void* user, const void* ca_file, const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        struct_fields fields(9);
        fields[0] = {"host", detail::text_value(host)};
        fields[1] = {"port", std::int64_t(5432)};
        fields[2] = {"database", detail::text_value(database)};
        fields[3] = {"user", detail::text_value(user)};
        fields[4] = {"ca_file", detail::text_value(ca_file)};
        fields[5] = {"connect_timeout_ms", std::int64_t(5000)};
        fields[6] = {"operation_timeout_ms", std::int64_t(30000)};
        fields[7] = {"max_value_bytes", std::int64_t(1048576)};
        fields[8] = {"read_only", false};
        *result = make_handle<std::any>(dynamic_struct(dynamic_struct_data{
            type_name, "postgres_config", std::move(fields)}));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_postgres_version(std::int64_t* result) noexcept
{
    *result = PQlibVersion();
    return 0;
}

extern "C" int txrt_db_open_postgres(const void* config, const void* password, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(db_open_postgres(postgres_config(config), input<secret::handle>(password)));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_sqlstate(const void* connection, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(db_sqlstate(input<db_connection>(connection)));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_transaction_status(const void* connection, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(db_transaction_status(input<db_connection>(connection)));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_pool(const void* config, std::int64_t capacity,
    std::int64_t waiters, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto& value = input<dynamic_struct>(config);
        db_options options{field<std::string>(value, 0), field<std::string>(value, 1),
            field<bool>(value, 2), field<bool>(value, 3),
            field<std::int64_t>(value, 4), field<std::int64_t>(value, 5)};
        *result = make_handle<std::any>(db_make_pool(options, capacity, waiters));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_postgres_pool(const void* config, const void* password,
    std::int64_t capacity, std::int64_t waiters, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(db_make_postgres_pool(postgres_config(config),
            input<secret::handle>(password), capacity, waiters));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_acquire(const void* pool, std::int64_t timeout_ms, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(db_acquire(input<db_pool>(pool), timeout_ms));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_release(const void* connection, bool* result) noexcept
{
    return invoke_checked([&]
    {
        *result = db_release(input<db_connection>(connection));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_close_pool(const void* pool, bool* result) noexcept
{
    return invoke_checked([&]
    {
        *result = db_close_pool(input<db_pool>(pool));
    }, tx::error_kind::database);
}
