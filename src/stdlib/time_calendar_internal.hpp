#pragma once

#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/value_format.hpp"

#include <any>
#include <cstdint>
#include <chrono>
#include <string>
#include <string_view>

namespace tx_generated::calendar
{

struct date_value
{
    std::int64_t year;
    std::int64_t month;
    std::int64_t day;
};

struct time_value
{
    std::int64_t hour;
    std::int64_t minute;
    std::int64_t second;
    std::int64_t microsecond;
};

struct offset_value
{
    std::int64_t unix_micros;
    std::int64_t offset_seconds;
};

struct zone_value
{
    std::int64_t unix_micros;
    std::string zone;
    std::int64_t offset_seconds;
};

[[noreturn]] void fail_parse(const char* code, const char* message);
[[noreturn]] void fail_runtime(const char* code, const char* message);
std::chrono::sys_days days_for(date_value value);
date_value date_for(std::chrono::sys_days value);
date_value parse_date_text(std::string_view text);
time_value parse_time_text(std::string_view text);
std::int64_t parse_offset_text(std::string_view text);
const dynamic_struct_data& object_at(const void* value,
                                      const char* display_name,
                                      std::size_t field_count);
std::int64_t number_at(const dynamic_struct_data& object, std::size_t index);
std::string text_at(const dynamic_struct_data& object, std::size_t index);

date_value read_date(const void* value);
time_value read_time(const void* value);
offset_value read_offset(const void* value);
zone_value read_zone(const void* value);
void* make_date(const char* type_name, date_value value);
void* make_time(const char* type_name, time_value value);
void* make_offset(const char* type_name, offset_value value);
void* make_zone(const char* type_name, zone_value value);
void* make_duration(const char* type_name, std::int64_t micros);

void validate_date(date_value value);
void validate_time(time_value value);
void validate_offset(std::int64_t seconds);
std::int64_t epoch_micros(date_value date, time_value time,
                          std::int64_t offset_seconds);
void local_from_epoch(std::int64_t epoch_micros, std::int64_t offset_seconds,
                      date_value& date, time_value& time);
std::string format_date(date_value value);
std::string format_time(time_value value);
std::string format_offset(offset_value value);

} // namespace tx_generated::calendar
