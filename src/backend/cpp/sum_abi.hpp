#pragma once

#include <cstdint>

extern "C"
{

int txrt_option_new(const char* type_name, bool present, const void* value,
                    void** result) noexcept;
int txrt_option_new_i64(const char* type_name, bool present,
                        std::int64_t value, void** result) noexcept;
int txrt_option_new_f64(const char* type_name, bool present,
                        double value, void** result) noexcept;
int txrt_option_new_bool(const char* type_name, bool present,
                         bool value, void** result) noexcept;
int txrt_result_new(const char* type_name, bool success, const void* value,
                    void** result) noexcept;
int txrt_result_from_legacy(const char* type_name, const void* legacy,
                             void** result) noexcept;
bool txrt_sum_state(const void* value) noexcept;
int txrt_option_value(const void* value, void** result) noexcept;
int txrt_option_value_i64(const void* value, std::int64_t* result) noexcept;
int txrt_option_value_f64(const void* value, double* result) noexcept;
int txrt_option_value_bool(const void* value, bool* result) noexcept;
int txrt_option_unpack_i64(const void* source, bool* present,
                           std::int64_t* value) noexcept;
int txrt_option_unpack_f64(const void* source, bool* present,
                           double* value) noexcept;
int txrt_option_unpack_bool(const void* source, bool* present,
                            bool* value) noexcept;
int txrt_option_empty_error() noexcept;
int txrt_option_value_or(const void* value, const void* fallback,
                          void** result) noexcept;
int txrt_result_value(const void* value, void** result) noexcept;
int txrt_result_error(const void* value, void** result) noexcept;

}
