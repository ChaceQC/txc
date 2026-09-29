#pragma once

#include "common/format_spec.hpp"

extern "C"
{

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
