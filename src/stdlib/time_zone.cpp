#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/time_calendar_internal.hpp"

#include <unicode/ucal.h>
#include <unicode/ustring.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace tx_generated::calendar
{
namespace
{

using calendar_handle = std::unique_ptr<UCalendar, decltype(&ucal_close)>;

void require_icu(UErrorCode status)
{
    if (U_FAILURE(status))
    {
        fail_runtime("timezone_failed", "IANA 时区数据处理失败");
    }
}

std::vector<UChar> utf16(std::string_view value)
{
    if (value.empty() || value.size() > 128 ||
        value.find('\0') != std::string_view::npos)
    {
        fail_parse("invalid_zone", "IANA 区域名无效");
    }
    UErrorCode status = U_ZERO_ERROR;
    int32_t length = 0;
    u_strFromUTF8(nullptr, 0, &length, value.data(),
                  static_cast<int32_t>(value.size()), &status);
    if (status != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(status))
    {
        fail_parse("invalid_zone", "IANA 区域名不是有效 UTF-8");
    }
    std::vector<UChar> result(static_cast<std::size_t>(length) + 1);
    status = U_ZERO_ERROR;
    u_strFromUTF8(result.data(), static_cast<int32_t>(result.size()), &length,
                  value.data(), static_cast<int32_t>(value.size()), &status);
    require_icu(status);
    return result;
}

std::string utf8(const UChar* value, int32_t length)
{
    UErrorCode status = U_ZERO_ERROR;
    int32_t bytes = 0;
    u_strToUTF8(nullptr, 0, &bytes, value, length, &status);
    if (status != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(status))
    {
        require_icu(status);
    }
    std::string result(static_cast<std::size_t>(bytes), '\0');
    status = U_ZERO_ERROR;
    u_strToUTF8(result.data(), bytes, nullptr, value, length, &status);
    require_icu(status);
    return result;
}

std::string canonical_zone(std::string_view name)
{
    const auto encoded = utf16(name);
    UChar canonical[256]{};
    UBool is_system = false;
    UErrorCode status = U_ZERO_ERROR;
    const auto length = ucal_getCanonicalTimeZoneID(encoded.data(),
        static_cast<int32_t>(encoded.size() - 1), canonical, 256,
        &is_system, &status);
    if (U_FAILURE(status) || !is_system || length <= 0)
    {
        fail_parse("invalid_zone", "未知 IANA 时区");
    }
    return utf8(canonical, length);
}

calendar_handle open_calendar(std::string_view zone)
{
    const auto encoded = utf16(zone);
    UErrorCode status = U_ZERO_ERROR;
    calendar_handle result(ucal_open(encoded.data(),
        static_cast<int32_t>(encoded.size() - 1), "en_US_POSIX",
        UCAL_GREGORIAN, &status), &ucal_close);
    require_icu(status);
    if (!result)
    {
        fail_runtime("timezone_failed", "无法打开 IANA 时区日历");
    }
    return result;
}

std::int64_t floor_millis(std::int64_t micros)
{
    auto millis = micros / 1000;
    if (micros % 1000 < 0)
    {
        --millis;
    }
    return millis;
}

std::int64_t offset_at(UCalendar* calendar, std::int64_t millis)
{
    UErrorCode status = U_ZERO_ERROR;
    ucal_setMillis(calendar, static_cast<UDate>(millis), &status);
    require_icu(status);
    const auto raw = ucal_get(calendar, UCAL_ZONE_OFFSET, &status);
    const auto summer = ucal_get(calendar, UCAL_DST_OFFSET, &status);
    require_icu(status);
    if ((raw + summer) % 1000 != 0)
    {
        fail_runtime("timezone_failed", "时区偏移无法以整秒表示");
    }
    return (raw + summer) / 1000;
}

std::int64_t wall_candidate(std::string_view zone, date_value date,
                            time_value time, UCalendarWallTimeOption option)
{
    auto calendar = open_calendar(zone);
    ucal_setAttribute(calendar.get(), UCAL_REPEATED_WALL_TIME, option);
    ucal_setAttribute(calendar.get(), UCAL_SKIPPED_WALL_TIME, option);
    ucal_clear(calendar.get());
    UErrorCode status = U_ZERO_ERROR;
    ucal_setDateTime(calendar.get(), static_cast<int32_t>(date.year),
        static_cast<int32_t>(date.month - 1), static_cast<int32_t>(date.day),
        static_cast<int32_t>(time.hour), static_cast<int32_t>(time.minute),
        static_cast<int32_t>(time.second), &status);
    ucal_set(calendar.get(), UCAL_MILLISECOND,
             static_cast<int32_t>(time.microsecond / 1000));
    require_icu(status);
    const auto millis = ucal_getMillis(calendar.get(), &status);
    require_icu(status);
    return static_cast<std::int64_t>(millis);
}

bool matches_wall(std::string_view zone, std::int64_t millis,
                  date_value date, time_value time)
{
    auto calendar = open_calendar(zone);
    UErrorCode status = U_ZERO_ERROR;
    ucal_setMillis(calendar.get(), static_cast<UDate>(millis), &status);
    const bool matches =
        ucal_get(calendar.get(), UCAL_YEAR, &status) == date.year &&
        ucal_get(calendar.get(), UCAL_MONTH, &status) + 1 == date.month &&
        ucal_get(calendar.get(), UCAL_DATE, &status) == date.day &&
        ucal_get(calendar.get(), UCAL_HOUR_OF_DAY, &status) == time.hour &&
        ucal_get(calendar.get(), UCAL_MINUTE, &status) == time.minute &&
        ucal_get(calendar.get(), UCAL_SECOND, &status) == time.second &&
        ucal_get(calendar.get(), UCAL_MILLISECOND, &status) ==
            time.microsecond / 1000;
    require_icu(status);
    return matches;
}

zone_value to_zone(std::int64_t unix_micros, std::string_view name)
{
    const auto zone = canonical_zone(name);
    auto calendar = open_calendar(zone);
    const auto offset = offset_at(calendar.get(), floor_millis(unix_micros));
    date_value date{};
    time_value time{};
    local_from_epoch(unix_micros, offset, date, time);
    return {unix_micros, zone, offset};
}

zone_value resolve(date_value date, time_value time,
                   std::string_view name, std::string_view policy)
{
    validate_date(date);
    validate_time(time);
    if (policy != "reject" && policy != "earlier" && policy != "later")
    {
        fail_runtime("invalid_argument", "时区歧义策略只能是 reject、earlier 或 later");
    }
    const auto zone = canonical_zone(name);
    const auto first = wall_candidate(zone, date, time, UCAL_WALLTIME_FIRST);
    const auto last = wall_candidate(zone, date, time, UCAL_WALLTIME_LAST);
    const bool first_matches = matches_wall(zone, first, date, time);
    const bool last_matches = matches_wall(zone, last, date, time);
    if (!first_matches && !last_matches && policy == "reject")
    {
        fail_parse("nonexistent_time", "本地时刻位于夏令时跳变缺口");
    }
    if (first_matches && last_matches && first != last && policy == "reject")
    {
        fail_parse("ambiguous_time", "本地时刻在夏令时回拨时重复");
    }
    if (first_matches != last_matches)
    {
        fail_runtime("timezone_failed", "时区候选的本地时间结果不一致");
    }
    // 两种策略按绝对时间选前/后一次，缺失时刻则归一化到缺口两侧。
    const auto chosen = policy == "later" ? std::max(first, last)
        : std::min(first, last);
    auto calendar = open_calendar(zone);
    const auto offset = offset_at(calendar.get(), chosen);
    return {chosen * 1000 + time.microsecond % 1000, zone, offset};
}

} // namespace

extern "C" int txrt_time_timezone_version(void** result) noexcept
{
    return detail::invoke_checked([&]
    {
        UErrorCode status = U_ZERO_ERROR;
        const char* version = ucal_getTZDataVersion(&status);
        require_icu(status);
        *result = detail::make_handle<std::string>(std::string(version));
    });
}

extern "C" int txrt_time_at_zone(const void* value, const void* name,
    const char* type_name, void** result) noexcept
{
    return detail::invoke_checked([&]
    {
        const auto& zone = tx_generated::detail::text_value(name);
        *result = make_zone(type_name,
            to_zone(read_offset(value).unix_micros, zone));
    });
}

extern "C" int txrt_time_resolve_local(const void* date, const void* time,
    const void* name, const void* policy, const char* type_name,
    void** result) noexcept
{
    return detail::invoke_checked([&]
    {
        *result = make_zone(type_name, resolve(read_date(date), read_time(time),
            tx_generated::detail::text_value(name),
            tx_generated::detail::text_value(policy)));
    });
}

extern "C" int txrt_time_format_zoned_datetime(const void* value,
    void** result) noexcept
{
    return detail::invoke_checked([&]
    {
        const auto input = read_zone(value);
        const auto actual = to_zone(input.unix_micros, input.zone);
        if (actual.offset_seconds != input.offset_seconds)
        {
            fail_runtime("invalid_argument", "时区时间的偏移与 IANA 数据不符");
        }
        *result = detail::make_handle<std::string>(format_offset(
            {actual.unix_micros, actual.offset_seconds}) + "[" + actual.zone + "]");
    });
}

extern "C" int txrt_time_zoned_local_date(const void* value,
    const char* type_name, void** result) noexcept
{
    return detail::invoke_checked([&]
    {
        const auto input = read_zone(value);
        const auto actual = to_zone(input.unix_micros, input.zone);
        date_value date{};
        time_value time{};
        local_from_epoch(actual.unix_micros, actual.offset_seconds, date, time);
        *result = make_date(type_name, date);
    });
}

extern "C" int txrt_time_zoned_local_time(const void* value,
    const char* type_name, void** result) noexcept
{
    return detail::invoke_checked([&]
    {
        const auto input = read_zone(value);
        const auto actual = to_zone(input.unix_micros, input.zone);
        date_value date{};
        time_value time{};
        local_from_epoch(actual.unix_micros, actual.offset_seconds, date, time);
        *result = make_time(type_name, time);
    });
}

} // namespace tx_generated::calendar
