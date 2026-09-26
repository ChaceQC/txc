#include "stdlib/time_calendar_internal.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <limits>
#include <string_view>
#include <utility>

namespace tx_generated::calendar
{

constexpr std::int64_t day_micros = 86400000000LL;
constexpr std::int64_t second_micros = 1000000LL;

std::int64_t digits(std::string_view text, std::size_t start, std::size_t count)
{
    if (start + count > text.size())
    {
        fail_parse("invalid_syntax", "ISO 8601 字段长度不足");
    }
    std::int64_t result = 0;
    for (std::size_t index = start; index < start + count; ++index)
    {
        if (text[index] < '0' || text[index] > '9')
        {
            fail_parse("invalid_syntax", "ISO 8601 字段需要 ASCII 数字");
        }
        result = result * 10 + text[index] - '0';
    }
    return result;
}

std::chrono::sys_days days_for(date_value value)
{
    return std::chrono::sys_days{std::chrono::year{static_cast<int>(value.year)} /
        std::chrono::month{static_cast<unsigned>(value.month)} /
        std::chrono::day{static_cast<unsigned>(value.day)}};
}

date_value date_for(std::chrono::sys_days value)
{
    const std::chrono::year_month_day parts{value};
    const date_value result{static_cast<int>(parts.year()),
        static_cast<unsigned>(parts.month()), static_cast<unsigned>(parts.day())};
    validate_date(result);
    return result;
}

date_value parse_date_text(std::string_view text)
{
    if (text.size() != 10 || text[4] != '-' || text[7] != '-')
    {
        fail_parse("invalid_syntax", "日期需要 YYYY-MM-DD 格式");
    }
    const date_value value{digits(text, 0, 4), digits(text, 5, 2),
        digits(text, 8, 2)};
    const auto valid = value.year >= 1 && value.year <= 9999 &&
        std::chrono::year_month_day{std::chrono::year{static_cast<int>(value.year)},
            std::chrono::month{static_cast<unsigned>(value.month)},
            std::chrono::day{static_cast<unsigned>(value.day)}}.ok();
    if (!valid)
    {
        fail_parse("out_of_range", "日期超出公历范围");
    }
    return value;
}

time_value parse_time_text(std::string_view text)
{
    if (text.size() < 8 || text[2] != ':' || text[5] != ':')
    {
        fail_parse("invalid_syntax", "时刻需要 HH:MM:SS 格式");
    }
    time_value result{digits(text, 0, 2), digits(text, 3, 2),
        digits(text, 6, 2), 0};
    if (text.size() > 8)
    {
        const auto precision = text.size() - 9;
        if (text[8] != '.' || precision == 0 || precision > 6)
        {
            fail_parse("invalid_syntax", "秒的小数部分需要 1～6 位");
        }
        result.microsecond = digits(text, 9, precision);
        for (std::size_t index = precision; index < 6; ++index)
        {
            result.microsecond *= 10;
        }
    }
    if (result.hour > 23 || result.minute > 59 || result.second > 59)
    {
        fail_parse("out_of_range", "时刻超出一天的范围");
    }
    return result;
}

std::int64_t parse_offset_text(std::string_view text)
{
    if (text == "Z")
    {
        return 0;
    }
    if ((text.size() != 6 && text.size() != 9) ||
        (text[0] != '+' && text[0] != '-') || text[3] != ':' ||
        (text.size() == 9 && text[6] != ':'))
    {
        fail_parse("invalid_syntax", "UTC 偏移需要 Z 或 ±HH:MM[:SS]");
    }
    const auto hour = digits(text, 1, 2);
    const auto minute = digits(text, 4, 2);
    const auto second = text.size() == 9 ? digits(text, 7, 2) : 0;
    if (hour > 18 || minute > 59 || second > 59 ||
        (hour == 18 && (minute != 0 || second != 0)))
    {
        fail_parse("out_of_range", "UTC 偏移超出范围");
    }
    const auto magnitude = hour * 3600 + minute * 60 + second;
    return text[0] == '-' ? -magnitude : magnitude;
}

std::string padded(std::int64_t value, int width)
{
    char buffer[32]{};
    std::snprintf(buffer, sizeof(buffer), "%0*lld", width,
                  static_cast<long long>(value));
    return buffer;
}

[[noreturn]] void fail_parse(const char* code, const char* message)
{
    throw runtime_failure({tx::error_kind::parse, code, message});
}

[[noreturn]] void fail_runtime(const char* code, const char* message)
{
    throw runtime_failure({tx::error_kind::runtime, code, message});
}

const dynamic_struct_data& object_at(const void* value,
                                      const char* display_name,
                                      std::size_t field_count)
{
    const auto& object = std::any_cast<const dynamic_struct&>(
        *static_cast<const std::any*>(value));
    if (object->display_name != display_name || object->fields.size() != field_count)
    {
        fail_runtime("invalid_argument", "日历值结构不匹配");
    }
    return *object.data;
}

std::int64_t number_at(const dynamic_struct_data& object, std::size_t index)
{
    return std::any_cast<std::int64_t>(object.fields[index].value);
}

std::string text_at(const dynamic_struct_data& object, std::size_t index)
{
    return std::any_cast<std::string>(object.fields[index].value);
}

void validate_date(date_value value)
{
    if (value.year < 1 || value.year > 9999 ||
        !std::chrono::year_month_day{std::chrono::year{static_cast<int>(value.year)},
            std::chrono::month{static_cast<unsigned>(value.month)},
            std::chrono::day{static_cast<unsigned>(value.day)}}.ok())
    {
        fail_runtime("out_of_range", "日期超出公历范围");
    }
}

void validate_time(time_value value)
{
    if (value.hour < 0 || value.hour > 23 || value.minute < 0 ||
        value.minute > 59 || value.second < 0 || value.second > 59 ||
        value.microsecond < 0 || value.microsecond >= second_micros)
    {
        fail_runtime("out_of_range", "时刻超出一天的范围");
    }
}

void validate_offset(std::int64_t seconds)
{
    if (seconds < -18 * 3600 || seconds > 18 * 3600)
    {
        fail_runtime("out_of_range", "UTC 偏移超出范围");
    }
}

date_value read_date(const void* value)
{
    const auto& item = object_at(value, "local_date", 3);
    const date_value result{number_at(item, 0), number_at(item, 1),
        number_at(item, 2)};
    validate_date(result);
    return result;
}

time_value read_time(const void* value)
{
    const auto& item = object_at(value, "local_time", 4);
    const time_value result{number_at(item, 0), number_at(item, 1),
        number_at(item, 2), number_at(item, 3)};
    validate_time(result);
    return result;
}

offset_value read_offset(const void* value)
{
    const auto& item = object_at(value, "offset_datetime", 2);
    const offset_value result{number_at(item, 0), number_at(item, 1)};
    validate_offset(result.offset_seconds);
    return result;
}

zone_value read_zone(const void* value)
{
    const auto& item = object_at(value, "zoned_datetime", 3);
    const zone_value result{number_at(item, 0), text_at(item, 1),
        number_at(item, 2)};
    validate_offset(result.offset_seconds);
    return result;
}

void* make_date(const char* type_name, date_value value)
{
    struct_fields fields(3);
    fields[0] = {"year", value.year};
    fields[1] = {"month", value.month};
    fields[2] = {"day", value.day};
    return detail::make_handle<std::any>(dynamic_struct({type_name,
        "local_date", std::move(fields)}));
}

void* make_time(const char* type_name, time_value value)
{
    struct_fields fields(4);
    fields[0] = {"hour", value.hour};
    fields[1] = {"minute", value.minute};
    fields[2] = {"second", value.second};
    fields[3] = {"microsecond", value.microsecond};
    return detail::make_handle<std::any>(dynamic_struct({type_name,
        "local_time", std::move(fields)}));
}

void* make_offset(const char* type_name, offset_value value)
{
    struct_fields fields(2);
    fields[0] = {"unix_micros", value.unix_micros};
    fields[1] = {"offset_seconds", value.offset_seconds};
    return detail::make_handle<std::any>(dynamic_struct({type_name,
        "offset_datetime", std::move(fields)}));
}

void* make_zone(const char* type_name, zone_value value)
{
    struct_fields fields(3);
    fields[0] = {"unix_micros", value.unix_micros};
    fields[1] = {"zone", std::move(value.zone)};
    fields[2] = {"offset_seconds", value.offset_seconds};
    return detail::make_handle<std::any>(dynamic_struct({type_name,
        "zoned_datetime", std::move(fields)}));
}

void* make_duration(const char* type_name, std::int64_t micros)
{
    struct_fields fields(1);
    fields[0] = {"micros", micros};
    return detail::make_handle<std::any>(dynamic_struct({type_name,
        "duration", std::move(fields)}));
}

std::int64_t epoch_micros(date_value date, time_value time,
                          std::int64_t offset_seconds)
{
    validate_date(date);
    validate_time(time);
    validate_offset(offset_seconds);
    return days_for(date).time_since_epoch().count() * day_micros +
        time.hour * 3600 * second_micros +
        time.minute * 60 * second_micros +
        time.second * second_micros + time.microsecond -
        offset_seconds * second_micros;
}

void local_from_epoch(std::int64_t epoch, std::int64_t offset_seconds,
                      date_value& date, time_value& time)
{
    validate_offset(offset_seconds);
    const auto shift = offset_seconds * second_micros;
    if ((shift > 0 && epoch > std::numeric_limits<std::int64_t>::max() - shift) ||
        (shift < 0 && epoch < std::numeric_limits<std::int64_t>::min() - shift))
    {
        fail_runtime("out_of_range", "日期时间超出可表示范围");
    }
    const auto local = epoch + shift;
    // 负 Unix 时间要向下取整到自然日，不能用 C++ 向零截断。
    auto day_index = local / day_micros;
    auto remaining = local % day_micros;
    if (remaining < 0)
    {
        --day_index;
        remaining += day_micros;
    }
    date = date_for(std::chrono::sys_days{
        std::chrono::days{static_cast<int>(day_index)}});
    time = {remaining / (3600 * second_micros),
        remaining / (60 * second_micros) % 60,
        remaining / second_micros % 60, remaining % second_micros};
}

std::string format_date(date_value value)
{
    validate_date(value);
    return padded(value.year, 4) + "-" + padded(value.month, 2) +
        "-" + padded(value.day, 2);
}

std::string format_time(time_value value)
{
    validate_time(value);
    std::string result = padded(value.hour, 2) + ":" +
        padded(value.minute, 2) + ":" + padded(value.second, 2);
    if (value.microsecond != 0)
    {
        auto fraction = padded(value.microsecond, 6);
        while (fraction.back() == '0')
        {
            fraction.pop_back();
        }
        result += "." + fraction;
    }
    return result;
}

std::string format_offset(offset_value value)
{
    date_value date{};
    time_value time{};
    local_from_epoch(value.unix_micros, value.offset_seconds, date, time);
    std::string result = format_date(date) + "T" + format_time(time);
    if (value.offset_seconds == 0)
    {
        return result + "Z";
    }
    const auto magnitude = value.offset_seconds < 0
        ? -value.offset_seconds : value.offset_seconds;
    result += value.offset_seconds < 0 ? "-" : "+";
    result += padded(magnitude / 3600, 2) + ":" +
        padded(magnitude / 60 % 60, 2);
    if (magnitude % 60 != 0)
    {
        result += ":" + padded(magnitude % 60, 2);
    }
    return result;
}


} // namespace tx_generated::calendar
