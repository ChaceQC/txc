#include "backend/cpp/db_abi.hpp"
#include "backend/cpp/db_abi_internal.hpp"
#include "backend/cpp/decimal_value.hpp"
#include "stdlib/time_calendar_internal.hpp"

using namespace tx_generated;
using namespace tx_generated::db_abi;
using tx_generated::detail::invoke_checked;

namespace
{

std::any take_value(void* value)
{
    std::unique_ptr<std::any, void(*)(std::any*)> owned(
        static_cast<std::any*>(value), detail::destroy_handle<std::any>);
    return *owned;
}

std::optional<std::any> decimal_option(const db_value& value, const char* type_name)
{
    const auto text = db_domain_text(value, db_value_kind::decimal);
    if (!text)
    {
        return std::nullopt;
    }
    try
    {
        return take_value(decimal_math::make(decimal_math::parse(*text), type_name));
    }
    catch (const runtime_failure&)
    {
        db_fail("type_mismatch", "数据库文本不是有效十进制值");
    }
}

std::optional<std::any> datetime_option(const db_value& value, const char* type_name)
{
    const auto text = db_domain_text(value, db_value_kind::datetime);
    if (!text)
    {
        return std::nullopt;
    }
    const std::string_view input(*text);
    const auto marker = input.find_first_of("Z+-", 11);
    if (input.size() < 20 || input[10] != 'T' || marker == std::string_view::npos)
    {
        db_fail("type_mismatch", "数据库文本不是带偏移时间");
    }
    try
    {
        const auto date = calendar::parse_date_text(input.substr(0, 10));
        const auto time = calendar::parse_time_text(input.substr(11, marker - 11));
        const auto offset = calendar::parse_offset_text(input.substr(marker));
        return take_value(calendar::make_offset(type_name,
            {calendar::epoch_micros(date, time, offset), offset}));
    }
    catch (const runtime_failure&)
    {
        db_fail("type_mismatch", "数据库文本不是有效带偏移时间");
    }
}

} // namespace

extern "C" int txrt_db_decimal_value(const void* value, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_value({db_value_kind::decimal,
            decimal_math::to_text(decimal_math::read(value))});
    }, tx::error_kind::database);
}

extern "C" int txrt_db_datetime_value(const void* value, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_value({db_value_kind::datetime,
            calendar::format_offset(calendar::read_offset(value))});
    }, tx::error_kind::database);
}

extern "C" int txrt_db_as_decimal(const void* value, const char* type_name,
    const char* element_type, void** result) noexcept
{
    return read_value(type_name, result, [&]
    {
        return decimal_option(input<db_value>(value), element_type);
    });
}

extern "C" int txrt_db_get_decimal(const void* row, std::int64_t index,
    const char* type_name, const char* element_type, void** result) noexcept
{
    return read_value(type_name, result, [&]
    {
        return decimal_option(db_column(input<db_row>(row), index), element_type);
    });
}

extern "C" int txrt_db_as_datetime(const void* value, const char* type_name,
    const char* element_type, void** result) noexcept
{
    return read_value(type_name, result, [&]
    {
        return datetime_option(input<db_value>(value), element_type);
    });
}

extern "C" int txrt_db_get_datetime(const void* row, std::int64_t index,
    const char* type_name, const char* element_type, void** result) noexcept
{
    return read_value(type_name, result, [&]
    {
        return datetime_option(db_column(input<db_row>(row), index), element_type);
    });
}
