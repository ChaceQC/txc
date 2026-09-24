#pragma once

#include <cstdint>
#include <cstddef>

// LLVM 生成的程序只通过这些 C ABI 符号调用可能报错的基础运算。
// 返回 0 表示成功；失败时可读取当前线程的错误文本。
extern "C"
{

const char* txrt_last_error() noexcept;
void txrt_require_success(int status) noexcept;
int txrt_prepare_console() noexcept;
int txrt_print_i64(std::int64_t value) noexcept;
int txrt_print_f64(double value) noexcept;
int txrt_print_bool(bool value) noexcept;
int txrt_exit_code(std::int64_t value) noexcept;
int txrt_float_to_int(double value, std::int64_t* result) noexcept;
// str 句柄由返回方持有；clone 返回独立副本，release 释放句柄。
int txrt_str_new(const char* bytes, std::size_t length, void** result) noexcept;
int txrt_str_clone(const void* value, void** result) noexcept;
void txrt_str_release(void* value) noexcept;
int txrt_str_concat(const void* left, const void* right,
                    void** result) noexcept;
int txrt_str_compare(const void* left, const void* right,
                     int* result) noexcept;
int txrt_str_len(const void* value, std::int64_t* result) noexcept;
int txrt_print_str(const void* value) noexcept;
int txrt_input(const void* prompt, void** result) noexcept;
int txrt_parse_int(const void* value, std::int64_t* result) noexcept;
int txrt_parse_float(const void* value, double* result) noexcept;
int txrt_int_to_str(std::int64_t value, void** result) noexcept;
int txrt_float_to_str(double value, void** result) noexcept;
int txrt_bool_to_str(bool value, void** result) noexcept;
int txrt_add_i64(std::int64_t left, std::int64_t right,
                 std::int64_t* result) noexcept;
int txrt_sub_i64(std::int64_t left, std::int64_t right,
                 std::int64_t* result) noexcept;
int txrt_mul_i64(std::int64_t left, std::int64_t right,
                 std::int64_t* result) noexcept;
int txrt_div_i64(std::int64_t left, std::int64_t right,
                 std::int64_t* result) noexcept;
int txrt_neg_i64(std::int64_t value, std::int64_t* result) noexcept;
int txrt_div_f64(double left, double right, double* result) noexcept;

}
