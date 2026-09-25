#pragma once

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string_view>

using clock_type = std::chrono::steady_clock;
using time_point = clock_type::time_point;

inline void report(std::string_view name, time_point started,
                   std::int64_t checksum)
{
    const auto elapsed = std::chrono::duration<double, std::milli>(
        clock_type::now() - started).count();
    std::cout << name << '\n' << elapsed << '\n' << checksum << '\n';
}

void bench_scalar_control();
void bench_updates();
void bench_while_logic();
void bench_float_arithmetic();
void bench_overloads();
void bench_named_arguments();
void bench_recursion();
void bench_variadic_unpack();
void bench_array_destructure();
void bench_array_padded();
void bench_dict_iteration();
void bench_struct_operators();
void bench_class_methods();
void bench_virtual_interface();
void bench_class_operator();
void bench_runtime_cast();
void bench_module_call();
void bench_string_conversion();
void bench_deep_copy();
void bench_copy_cycle();
void bench_deinit();
void bench_cycle_gc();
