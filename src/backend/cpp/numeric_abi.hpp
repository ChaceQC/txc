#pragma once

#include <cstdint>

// 类型已由前端检查；状态码函数失败时交给 txrt_require_success。
// 随机数上下文函数接收 runtime_context，返回状态并通过输出参数交付结果。
extern "C"
{

int txrt_math_abs_i64(std::int64_t value, std::int64_t* result) noexcept;
int txrt_math_abs_f64(double value, double* result) noexcept;
int txrt_math_min_i64(std::int64_t left, std::int64_t right,
                      std::int64_t* result) noexcept;
int txrt_math_min_f64(double left, double right, double* result) noexcept;
int txrt_math_max_i64(std::int64_t left, std::int64_t right,
                      std::int64_t* result) noexcept;
int txrt_math_max_f64(double left, double right, double* result) noexcept;
int txrt_math_clamp_i64(std::int64_t value, std::int64_t lower,
                        std::int64_t upper, std::int64_t* result) noexcept;
int txrt_math_clamp_f64(double value, double lower, double upper,
                        double* result) noexcept;
int txrt_math_mod_i64(std::int64_t left, std::int64_t right,
                      std::int64_t* result) noexcept;
int txrt_math_sqrt_f64(double value, double* result) noexcept;
int txrt_math_pow_f64(double base, double exponent, double* result) noexcept;
int txrt_math_floor_f64(double value, std::int64_t* result) noexcept;
int txrt_math_ceil_f64(double value, std::int64_t* result) noexcept;
int txrt_math_sin(double value, double* result) noexcept;
int txrt_math_cos(double value, double* result) noexcept;
int txrt_math_tan(double value, double* result) noexcept;
int txrt_math_asin(double value, double* result) noexcept;
int txrt_math_acos(double value, double* result) noexcept;
int txrt_math_atan(double value, double* result) noexcept;
int txrt_math_atan2(double y, double x, double* result) noexcept;
int txrt_math_sinh(double value, double* result) noexcept;
int txrt_math_cosh(double value, double* result) noexcept;
int txrt_math_tanh(double value, double* result) noexcept;
int txrt_math_exp(double value, double* result) noexcept;
int txrt_math_log(double value, double* result) noexcept;
int txrt_math_log10(double value, double* result) noexcept;
int txrt_math_is_finite(double value, bool* result) noexcept;
int txrt_math_is_nan(double value, bool* result) noexcept;
int txrt_math_is_infinite(double value, bool* result) noexcept;
int txrt_math_gcd(std::int64_t left, std::int64_t right,
                  std::int64_t* result) noexcept;
int txrt_math_lcm(std::int64_t left, std::int64_t right,
                  std::int64_t* result) noexcept;
int txrt_math_pow_int(std::int64_t base, std::int64_t exponent,
                      std::int64_t* result) noexcept;
int txrt_math_round_to_int(double value, const void* mode,
                           std::int64_t* result) noexcept;
int txrt_random_seed_i64(std::int64_t value) noexcept;
int txrt_random_int_i64(std::int64_t lower, std::int64_t upper,
                        std::int64_t* result) noexcept;
int txrt_random_float_f64(double* result) noexcept;
std::int64_t txrt_random_int_direct(std::int64_t lower,
                                    std::int64_t upper) noexcept;
double txrt_random_float_direct() noexcept;
void* txrt_random_context() noexcept;
int txrt_random_seed_context(void* context, std::int64_t value) noexcept;
int txrt_random_int_context(void* context, std::int64_t lower,
                            std::int64_t upper, std::int64_t* result) noexcept;
int txrt_random_float_context(void* context, double* result) noexcept;

}
