#include "backend/cpp/db_abi.hpp"
#include "backend/cpp/db_abi_internal.hpp"
#include <sqlite3.h>

using namespace tx_generated;
using namespace tx_generated::db_abi;
using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

extern "C" int txrt_db_sqlite_options(const void* path,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        struct_fields fields(6);
        fields[0] = {"driver", std::string("sqlite")};
        fields[1] = {"path", detail::text_value(path)};
        fields[2] = {"read_only", false};
        fields[3] = {"wal", false};
        fields[4] = {"busy_timeout_ms", std::int64_t(5000)};
        fields[5] = {"max_value_bytes", std::int64_t(1048576)};
        *result = make_handle<std::any>(dynamic_struct(dynamic_struct_data{
            type_name, "options", std::move(fields)}));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_sqlite_version(void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(sqlite3_libversion());
    });
}

extern "C" int txrt_db_open(const void* config, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto& value = input<dynamic_struct>(config);
        db_options options{field<std::string>(value, 0), field<std::string>(value, 1),
            field<bool>(value, 2), field<bool>(value, 3),
            field<std::int64_t>(value, 4), field<std::int64_t>(value, 5)};
        *result = make_handle<std::any>(db_open(options));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_bind_all(const void* statement, const void* params) noexcept
{
    return invoke_checked([&]
    {
        const auto& values = field<object_vector>(input<dynamic_struct>(params), 0);
        std::vector<db_value> converted;
        converted.reserve(values.data().values.size());
        for (const auto& value : values.data().values)
        {
            converted.push_back(std::any_cast<const db_value&>(value));
        }
        db_bind_all(input<db_statement>(statement), converted);
    }, tx::error_kind::database);
}

extern "C" int txrt_db_execute(const void* statement,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto executed = db_execute(input<db_statement>(statement));
        std::unique_ptr<std::any, void(*)(std::any*)> id(
            static_cast<std::any*>(option("option<int>", executed.insert_id)),
            detail::destroy_handle<std::any>);
        struct_fields fields(2);
        fields[0] = {"affected_rows", executed.affected_rows};
        fields[1] = {"insert_id", *id};
        *result = make_handle<std::any>(dynamic_struct(dynamic_struct_data{
            type_name, "execution", std::move(fields)}));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_next(const void* cursor,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto row = db_next(input<db_cursor>(cursor));
        *result = option(type_name, row ? std::optional<db_row>(row) : std::nullopt);
    }, tx::error_kind::database);
}
