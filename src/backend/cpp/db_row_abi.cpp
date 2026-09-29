#include "backend/cpp/db_abi.hpp"
#include "backend/cpp/db_abi_internal.hpp"

using namespace tx_generated;
using namespace tx_generated::db_abi;
using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

extern "C" int txrt_db_column_count(const void* row, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        const auto& value = input<db_row>(row);
        if (!value)
        {
            db_fail("invalid_state", "数据库行句柄无效");
        }
        *result = static_cast<std::int64_t>(value->values.size());
    }, tx::error_kind::database);
}

extern "C" int txrt_db_column_name(const void* row, std::int64_t index,
    void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto& value = input<db_row>(row);
        db_column(value, index);
        *result = make_handle<std::string>(value->names[static_cast<std::size_t>(index)]);
    }, tx::error_kind::database);
}

extern "C" int txrt_db_column_index(const void* row, const void* name,
    std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = db_column_index(input<db_row>(row), detail::text_value(name));
    }, tx::error_kind::database);
}

extern "C" int txrt_db_get_value(const void* row, std::int64_t index,
    void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(db_column(input<db_row>(row), index));
    }, tx::error_kind::database);
}
