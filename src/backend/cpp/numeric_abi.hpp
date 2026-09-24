#pragma once

#include <cstdint>

// 类型已由前端检查；状态码函数失败时交给 txrt_require_success。
// 随机数上下文函数直接返回结果，异常时输出运行错误并退出。
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
int txrt_random_seed_i64(std::int64_t value) noexcept;
int txrt_random_int_i64(std::int64_t lower, std::int64_t upper,
                        std::int64_t* result) noexcept;
int txrt_random_float_f64(double* result) noexcept;
std::int64_t txrt_random_int_direct(std::int64_t lower,
                                    std::int64_t upper) noexcept;
double txrt_random_float_direct() noexcept;
void* txrt_random_context() noexcept;
int txrt_random_seed_context(void* context, std::int64_t value) noexcept;
std::int64_t txrt_random_int_context(void* context, std::int64_t lower,
                                     std::int64_t upper) noexcept;
double txrt_random_float_context(void* context) noexcept;

}
