#pragma once

#include <cstdint>

extern "C"
{

int txrt_parse_int_scalar(const void* text, std::int64_t base,
    bool* ok, std::int64_t* value, std::int64_t* error) noexcept;
int txrt_parse_float_scalar(const void* text,
    bool* ok, double* value, std::int64_t* error) noexcept;
int txrt_parse_materialize_int(bool ok, std::int64_t value, std::int64_t error,
    const char* result_type, const char* error_type, void** result) noexcept;
int txrt_parse_materialize_float(bool ok, double value, std::int64_t error,
    const char* result_type, const char* error_type, void** result) noexcept;

int txrt_parse_try_parse_int(const void* text, std::int64_t base,
    const char* result_type, const char* error_type, void** result) noexcept;
int txrt_parse_try_parse_float(const void* text,
    const char* result_type, const char* error_type, void** result) noexcept;
int txrt_parse_parse_int(const void* text, std::int64_t base,
                         std::int64_t* result) noexcept;
int txrt_parse_parse_float(const void* text, double* result) noexcept;
int txrt_file_try_read_text(const void* path, const void* encoding,
    const char* result_type, const char* error_type, void** result) noexcept;
int txrt_file_try_write_text(const void* path, const void* text, const void* encoding,
    const char* result_type, const char* error_type, void** result) noexcept;
int txrt_file_try_append_text(const void* path, const void* text, const void* encoding,
    const char* result_type, const char* error_type, void** result) noexcept;

}
