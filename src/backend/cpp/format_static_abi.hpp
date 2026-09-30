#pragma once

#include "common/format_spec.hpp"
#include <string>

namespace tx_generated
{
struct static_format_step
{
    void (*append)(std::string&, std::uint64_t, const tx::format_spec&, char);
    std::uint64_t argument;
    tx::format_spec spec;
    std::uint64_t conversion;
    const char* tail;
    std::uint64_t length;
};
static_assert(sizeof(static_format_step) == 96);
}

extern "C"
{
std::int64_t txrt_format_integer_length(std::int64_t value) noexcept;
int txrt_format_literal_length(const char* value, std::uint64_t size, std::int64_t* result) noexcept;
int txrt_format_execute(const tx_generated::static_format_step* steps, std::uint64_t count,
    const std::uint64_t* arguments, const char* prefix, std::uint64_t length,
    std::uint64_t capacity, void** result) noexcept;
void tx_format_fast_i64(std::string&, std::uint64_t, const tx::format_spec&, char);
void tx_format_fast_bool(std::string&, std::uint64_t, const tx::format_spec&, char);
void tx_format_fast_str(std::string&, std::uint64_t, const tx::format_spec&, char);
void tx_format_fast_literal(std::string&, std::uint64_t, const tx::format_spec&, char);

int txrt_format_begin(void** result, std::uint64_t capacity,
    const char* text, std::uint64_t length) noexcept;
int txrt_format_append_i64(void* result, std::int64_t value,
    const tx::format_spec* spec, char conversion, const char* tail, std::uint64_t length) noexcept;
int txrt_format_append_f64(void* result, double value,
    const tx::format_spec* spec, char conversion, const char* tail, std::uint64_t length) noexcept;
int txrt_format_append_bool(void* result, bool value,
    const tx::format_spec* spec, char conversion, const char* tail, std::uint64_t length) noexcept;
int txrt_format_append_str(void* result, const void* value,
    const tx::format_spec* spec, char conversion, const char* tail, std::uint64_t length) noexcept;
int txrt_format_plain_i64(void* result, std::int64_t value,
    const char* tail, std::uint64_t length) noexcept;
int txrt_format_plain_bool(void* result, bool value,
    const char* tail, std::uint64_t length) noexcept;
int txrt_format_plain_str(void* result, const void* value,
    const char* tail, std::uint64_t length) noexcept;
int txrt_format_plain_bytes(void* result, const char* value, std::uint64_t size,
    const char* tail, std::uint64_t length) noexcept;
void txrt_format_finish(void* result) noexcept;

}
