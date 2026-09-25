#include "compare.hpp"

#include <iomanip>

int main()
{
    std::cout << std::setprecision(17);
    bench_scalar_control();
    bench_updates();
    bench_while_logic();
    bench_float_arithmetic();
    bench_overloads();
    bench_named_arguments();
    bench_recursion();
    bench_variadic_unpack();
    bench_array_destructure();
    bench_array_padded();
    bench_dict_iteration();
    bench_struct_operators();
    bench_class_methods();
    bench_virtual_interface();
    bench_class_operator();
    bench_runtime_cast();
    bench_module_call();
    bench_string_conversion();
    bench_deep_copy();
    bench_copy_cycle();
    bench_deinit();
    bench_cycle_gc();
}
