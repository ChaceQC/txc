#pragma once

#include <cstdint>

extern "C"
{

int txrt_json_parse(const void* text, void** result) noexcept;
int txrt_json_parse_object(const void* text, void** result) noexcept;
int txrt_json_try_parse(const void* text, const char* result_type,
                        const char* error_type, void** result) noexcept;
int txrt_json_stringify(const void* value, void** result) noexcept;
int txrt_json_stringify_pretty(const void* value, std::int64_t indent,
                               void** result) noexcept;
int txrt_json_contains(const void* object, const void* key,
                        bool* result) noexcept;
int txrt_json_get(const void* object, const void* key,
                   void** result) noexcept;
int txrt_json_get_int(const void* object, const void* key,
                       std::int64_t* result) noexcept;
int txrt_json_get_float(const void* object, const void* key,
                         double* result) noexcept;
int txrt_json_get_bool(const void* object, const void* key,
                        bool* result) noexcept;
int txrt_json_get_str(const void* object, const void* key,
                       void** result) noexcept;
int txrt_json_get_array(const void* object, const void* key,
                         void** result) noexcept;
int txrt_json_get_object(const void* object, const void* key,
                          void** result) noexcept;

}
