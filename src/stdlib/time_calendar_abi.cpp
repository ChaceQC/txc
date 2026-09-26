#include "stdlib/time_calendar_internal.hpp"

#include <algorithm>
#include <chrono>
#include <limits>
#include <string_view>

namespace tx_generated::calendar
{

extern "C" int txrt_time_parse_local_date(const void* text,
    const char* type_name, void** result) noexcept
{
    return detail::invoke_checked([&]
    {
        *result = make_date(type_name, parse_date_text(
            *static_cast<const std::string*>(text)));
    });
}

extern "C" int txrt_time_format_local_date(const void* value,
    void** result) noexcept
{
    return detail::invoke_checked([&]
    {
        *result = detail::make_handle<std::string>(format_date(read_date(value)));
    });
}

extern "C" int txrt_time_date_add_days(const void* value,
    std::int64_t count, const char* type_name, void** result) noexcept
{
    return detail::invoke_checked([&]
    {
        const auto start = days_for(read_date(value)).time_since_epoch().count();
        if (count < -719162 - start || count > 2932896 - start)
        {
            fail_runtime("out_of_range", "日历加减超出 0001～9999 年");
        }
        const auto finish = start + count;
        *result = make_date(type_name, date_for(std::chrono::sys_days{
            std::chrono::days{static_cast<int>(finish)}}));
    });
}

extern "C" int txrt_time_date_add_months(const void* value,
    std::int64_t count, const char* type_name, void** result) noexcept
{
    return detail::invoke_checked([&]
    {
        const auto date = read_date(value);
        const auto month_index = (date.year - 1) * 12 + date.month - 1;
        if (count < -month_index || count > 119987 - month_index)
        {
            fail_runtime("out_of_range", "月份加减超出 0001～9999 年");
        }
        const auto target = month_index + count;
        const auto year = static_cast<int>(target / 12 + 1);
        const auto month = static_cast<unsigned>(target % 12 + 1);
        const auto last = std::chrono::year_month_day_last{
            std::chrono::year{year}, std::chrono::month_day_last{
                std::chrono::month{month}}};
        *result = make_date(type_name, {year, month,
            std::min<std::int64_t>(date.day, static_cast<unsigned>(last.day()))});
    });
}

extern "C" int txrt_time_date_days_between(const void* start,
    const void* finish, std::int64_t* result) noexcept
{
    return detail::invoke_checked([&]
    {
        *result = (days_for(read_date(finish)) - days_for(read_date(start))).count();
    });
}

extern "C" int txrt_time_parse_local_time(const void* text,
    const char* type_name, void** result) noexcept
{
    return detail::invoke_checked([&]
    {
        *result = make_time(type_name, parse_time_text(
            *static_cast<const std::string*>(text)));
    });
}

extern "C" int txrt_time_format_local_time(const void* value,
    void** result) noexcept
{
    return detail::invoke_checked([&]
    {
        *result = detail::make_handle<std::string>(format_time(read_time(value)));
    });
}

extern "C" int txrt_time_parse_offset_datetime(const void* text,
    const char* type_name, void** result) noexcept
{
    return detail::invoke_checked([&]
    {
        const std::string_view input = *static_cast<const std::string*>(text);
        if (input.size() < 20 || input[10] != 'T')
        {
            fail_parse("invalid_syntax", "带偏移时间需要日期、T 和时刻");
        }
        const auto marker = input.find_first_of("Z+-", 11);
        if (marker == std::string_view::npos)
        {
            fail_parse("invalid_syntax", "带偏移时间缺少 UTC 偏移");
        }
        const auto date = parse_date_text(input.substr(0, 10));
        const auto time = parse_time_text(input.substr(11, marker - 11));
        const auto offset = parse_offset_text(input.substr(marker));
        *result = make_offset(type_name,
            {epoch_micros(date, time, offset), offset});
    });
}

extern "C" int txrt_time_offset_from_unix_millis(std::int64_t millis,
    std::int64_t offset, const char* type_name, void** result) noexcept
{
    return detail::invoke_checked([&]
    {
        validate_offset(offset);
        if (millis < std::numeric_limits<std::int64_t>::min() / 1000 ||
            millis > std::numeric_limits<std::int64_t>::max() / 1000)
        {
            fail_runtime("out_of_range", "Unix 毫秒超出微秒时间范围");
        }
        const offset_value value{millis * 1000, offset};
        format_offset(value);
        *result = make_offset(type_name, value);
    });
}

extern "C" int txrt_time_format_offset_datetime(const void* value,
    void** result) noexcept
{
    return detail::invoke_checked([&]
    {
        *result = detail::make_handle<std::string>(format_offset(read_offset(value)));
    });
}

extern "C" int txrt_time_datetime_difference(const void* start,
    const void* finish, const char* type_name, void** result) noexcept
{
    return detail::invoke_checked([&]
    {
        const auto left = read_offset(start).unix_micros;
        const auto right = read_offset(finish).unix_micros;
        if ((left < 0 && right > std::numeric_limits<std::int64_t>::max() + left) ||
            (left > 0 && right < std::numeric_limits<std::int64_t>::min() + left))
        {
            fail_runtime("out_of_range", "日期时间差超出时长范围");
        }
        *result = make_duration(type_name, right - left);
    });
}

} // namespace tx_generated::calendar
