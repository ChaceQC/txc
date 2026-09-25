#pragma once

#include <cstdint>

extern "C"
{

int txrt_algorithm_sort_i64(const void* values) noexcept;

int txrt_algorithm_sorted_i64(const void* values, void** result) noexcept;

int txrt_algorithm_find_i64(const void* values, std::int64_t value, std::int64_t* result) noexcept;

int txrt_algorithm_count_i64(const void* values, std::int64_t value, std::int64_t* result) noexcept;

int txrt_algorithm_lower_bound_i64(const void* values, std::int64_t value, std::int64_t* result) noexcept;

int txrt_algorithm_upper_bound_i64(const void* values, std::int64_t value, std::int64_t* result) noexcept;

int txrt_algorithm_reverse_i64(const void* values) noexcept;

int txrt_algorithm_sum_i64(const void* values, std::int64_t* result) noexcept;

int txrt_algorithm_min_element_i64(const void* values, std::int64_t* result) noexcept;

int txrt_algorithm_max_element_i64(const void* values, std::int64_t* result) noexcept;

int txrt_algorithm_sort_f64(const void* values) noexcept;

int txrt_algorithm_sorted_f64(const void* values, void** result) noexcept;

int txrt_algorithm_find_f64(const void* values, double value, std::int64_t* result) noexcept;

int txrt_algorithm_count_f64(const void* values, double value, std::int64_t* result) noexcept;

int txrt_algorithm_lower_bound_f64(const void* values, double value, std::int64_t* result) noexcept;

int txrt_algorithm_upper_bound_f64(const void* values, double value, std::int64_t* result) noexcept;

int txrt_algorithm_reverse_f64(const void* values) noexcept;

int txrt_algorithm_sum_f64(const void* values, double* result) noexcept;

int txrt_algorithm_min_element_f64(const void* values, double* result) noexcept;

int txrt_algorithm_max_element_f64(const void* values, double* result) noexcept;

int txrt_algorithm_sort_str(const void* values) noexcept;

int txrt_algorithm_sorted_str(const void* values, void** result) noexcept;

int txrt_algorithm_find_str(const void* values, const void* value, std::int64_t* result) noexcept;

int txrt_algorithm_count_str(const void* values, const void* value, std::int64_t* result) noexcept;

int txrt_algorithm_lower_bound_str(const void* values, const void* value, std::int64_t* result) noexcept;

int txrt_algorithm_upper_bound_str(const void* values, const void* value, std::int64_t* result) noexcept;

int txrt_algorithm_reverse_str(const void* values) noexcept;

int txrt_algorithm_find_bool(const void* values, bool value, std::int64_t* result) noexcept;

int txrt_algorithm_count_bool(const void* values, bool value, std::int64_t* result) noexcept;

int txrt_algorithm_reverse_bool(const void* values) noexcept;

}
