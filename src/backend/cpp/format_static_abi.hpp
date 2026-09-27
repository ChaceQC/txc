#pragma once

#include "common/format_spec.hpp"

extern "C"
{

int txrt_format_begin(void** result) noexcept;
int txrt_format_literal(void* result, const char* text, std::uint64_t length) noexcept;
int txrt_format_append_i64(void* result, std::int64_t value,
    const tx::format_spec* spec, char conversion) noexcept;
int txrt_format_append_f64(void* result, double value,
    const tx::format_spec* spec, char conversion) noexcept;
int txrt_format_append_bool(void* result, bool value,
    const tx::format_spec* spec, char conversion) noexcept;
int txrt_format_append_str(void* result, const void* value,
    const tx::format_spec* spec, char conversion) noexcept;

}
