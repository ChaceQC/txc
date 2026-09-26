#pragma once

extern "C"
{

int txrt_option_new(const char* type_name, bool present, const void* value,
                    void** result) noexcept;
int txrt_result_new(const char* type_name, bool success, const void* value,
                    void** result) noexcept;
int txrt_result_from_legacy(const char* type_name, const void* legacy,
                             void** result) noexcept;
bool txrt_sum_state(const void* value) noexcept;
int txrt_option_value(const void* value, void** result) noexcept;
int txrt_option_value_or(const void* value, const void* fallback,
                          void** result) noexcept;
int txrt_result_value(const void* value, void** result) noexcept;
int txrt_result_error(const void* value, void** result) noexcept;

}
