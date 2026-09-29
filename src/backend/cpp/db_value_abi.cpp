#include "backend/cpp/db_abi.hpp"
#include "backend/cpp/db_abi_internal.hpp"

using namespace tx_generated;
using namespace tx_generated::db_abi;
using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

extern "C" int txrt_db_null_value(void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_value({});
    }, tx::error_kind::database);
}

extern "C" int txrt_db_int_value(std::int64_t value, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_value({db_value_kind::integer, value});
    }, tx::error_kind::database);
}

extern "C" int txrt_db_float_value(double value, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_value({db_value_kind::floating, value});
    }, tx::error_kind::database);
}

extern "C" int txrt_db_bool_value(bool value, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_value({db_value_kind::boolean, value});
    }, tx::error_kind::database);
}

extern "C" int txrt_db_str_value(const void* value, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_value({db_value_kind::text, detail::text_value(value)});
    }, tx::error_kind::database);
}

extern "C" int txrt_db_bytes_value(const void* value, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_value({db_value_kind::binary, input<byte_value>(value)});
    }, tx::error_kind::database);
}

extern "C" int txrt_db_value_kind(const void* value, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(db_kind_name(input<db_value>(value)));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_as_int(const void* value,
    const char* type_name, void** result) noexcept
{
    return read_value(type_name, result, [&]
    {
        return db_as_int(input<db_value>(value));
    });
}

extern "C" int txrt_db_get_int(const void* row, std::int64_t index,
    const char* type_name, void** result) noexcept
{
    return read_value(type_name, result, [&]
    {
        return db_as_int(db_column(input<db_row>(row), index));
    });
}

extern "C" int txrt_db_as_float(const void* value,
    const char* type_name, void** result) noexcept
{
    return read_value(type_name, result, [&]
    {
        return db_as_float(input<db_value>(value));
    });
}

extern "C" int txrt_db_get_float(const void* row, std::int64_t index,
    const char* type_name, void** result) noexcept
{
    return read_value(type_name, result, [&]
    {
        return db_as_float(db_column(input<db_row>(row), index));
    });
}

extern "C" int txrt_db_as_bool(const void* value,
    const char* type_name, void** result) noexcept
{
    return read_value(type_name, result, [&]
    {
        return db_as_bool(input<db_value>(value));
    });
}

extern "C" int txrt_db_get_bool(const void* row, std::int64_t index,
    const char* type_name, void** result) noexcept
{
    return read_value(type_name, result, [&]
    {
        return db_as_bool(db_column(input<db_row>(row), index));
    });
}

extern "C" int txrt_db_as_str(const void* value,
    const char* type_name, void** result) noexcept
{
    return read_value(type_name, result, [&]
    {
        return db_as_str(input<db_value>(value));
    });
}

extern "C" int txrt_db_get_str(const void* row, std::int64_t index,
    const char* type_name, void** result) noexcept
{
    return read_value(type_name, result, [&]
    {
        return db_as_str(db_column(input<db_row>(row), index));
    });
}

extern "C" int txrt_db_as_bytes(const void* value,
    const char* type_name, void** result) noexcept
{
    return read_value(type_name, result, [&]
    {
        return db_as_bytes(input<db_value>(value));
    });
}

extern "C" int txrt_db_get_bytes(const void* row, std::int64_t index,
    const char* type_name, void** result) noexcept
{
    return read_value(type_name, result, [&]
    {
        return db_as_bytes(db_column(input<db_row>(row), index));
    });
}
