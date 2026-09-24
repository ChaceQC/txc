#include "backend/cpp/runtime_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"

#include "backend/cpp/runtime.hpp"

#include <any>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace tx_generated::detail
{

thread_local char last_error[256]{};

} // namespace tx_generated::detail

using tx_generated::detail::invoke_checked;

extern "C" const char* txrt_last_error() noexcept
{
    return tx_generated::detail::last_error;
}

extern "C" void txrt_require_success(int status) noexcept
{
    if (status != 0)
    {
        std::cerr << "运行错误：" << tx_generated::detail::last_error << '\n';
        std::exit(1);
    }
}

extern "C" int txrt_prepare_console() noexcept
{
    return invoke_checked([] { tx_generated::tx_prepare_console(); });
}

extern "C" int txrt_print_i64(std::int64_t value, bool newline) noexcept
{
    return invoke_checked([&] { tx_generated::tx_print(value, newline); });
}

extern "C" int txrt_print_f64(double value, bool newline) noexcept
{
    return invoke_checked([&] { tx_generated::tx_print(value, newline); });
}

extern "C" int txrt_print_bool(bool value, bool newline) noexcept
{
    return invoke_checked([&] { tx_generated::tx_print(value, newline); });
}

extern "C" int txrt_print_char(std::uint8_t value) noexcept
{
    return invoke_checked([&] {
        tx_generated::tx_fn_write(std::string(1, static_cast<char>(value)));
    });
}

extern "C" int txrt_exit_code(std::int64_t value) noexcept
{
    if (value < std::numeric_limits<int>::min() ||
        value > std::numeric_limits<int>::max())
    {
        std::cerr << "运行错误：main 返回码超出平台 int 范围\n";
        return 1;
    }
    return static_cast<int>(value);
}

extern "C" int txrt_float_to_int(double value,
                                   std::int64_t* result) noexcept
{
    return invoke_checked([&] { *result = tx_generated::tx_float_to_int(value); });
}

extern "C" int txrt_str_new(const char* bytes, std::size_t length,
                             void** result) noexcept
{
    return invoke_checked([&] { *result = new std::string(bytes, length); });
}

extern "C" int txrt_str_clone(const void* value, void** result) noexcept
{
    return invoke_checked([&] {
        *result = new std::string(*static_cast<const std::string*>(value));
    });
}

extern "C" void txrt_str_release(void* value) noexcept
{
    delete static_cast<std::string*>(value);
}

extern "C" int txrt_str_concat(const void* left, const void* right,
                                void** result) noexcept
{
    return invoke_checked([&] {
        *result = new std::string(*static_cast<const std::string*>(left) +
                                  *static_cast<const std::string*>(right));
    });
}

extern "C" int txrt_str_compare(const void* left, const void* right,
                                 int* result) noexcept
{
    return invoke_checked([&] {
        *result = static_cast<const std::string*>(left)->compare(
            *static_cast<const std::string*>(right));
    });
}

extern "C" int txrt_str_len(const void* value,
                             std::int64_t* result) noexcept
{
    return invoke_checked([&] {
        *result = tx_generated::tx_len(*static_cast<const std::string*>(value));
    });
}

extern "C" int txrt_print_str(const void* value, bool newline) noexcept
{
    return invoke_checked([&] {
        tx_generated::tx_print(*static_cast<const std::string*>(value), newline);
    });
}

extern "C" int txrt_input(const void* prompt, void** result) noexcept
{
    return invoke_checked([&] {
        *result = new std::string(prompt
            ? tx_generated::tx_input(*static_cast<const std::string*>(prompt))
            : tx_generated::tx_input());
    });
}

extern "C" int txrt_input_or_none(const void* prompt, void** result) noexcept
{
    return invoke_checked([&] {
        *result = new std::any(prompt
            ? tx_generated::tx_input_or_none(*static_cast<const std::string*>(prompt))
            : tx_generated::tx_input_or_none());
    });
}

extern "C" int txrt_parse_int(const void* value,
                                std::int64_t* result) noexcept
{
    return invoke_checked([&] {
        *result = tx_generated::tx_parse_int(*static_cast<const std::string*>(value));
    });
}

extern "C" int txrt_parse_float(const void* value, double* result) noexcept
{
    return invoke_checked([&] {
        *result = tx_generated::tx_parse_float(*static_cast<const std::string*>(value));
    });
}

extern "C" int txrt_int_to_str(std::int64_t value, void** result) noexcept
{
    return invoke_checked([&] {
        *result = new std::string(tx_generated::tx_int_to_string(value));
    });
}

extern "C" int txrt_float_to_str(double value, void** result) noexcept
{
    return invoke_checked([&] {
        *result = new std::string(tx_generated::tx_float_to_string(value));
    });
}

extern "C" int txrt_bool_to_str(bool value, void** result) noexcept
{
    return invoke_checked([&] {
        *result = new std::string(tx_generated::tx_bool_to_string(value));
    });
}

extern "C" int txrt_add_i64(std::int64_t left, std::int64_t right,
                             std::int64_t* result) noexcept
{
    return invoke_checked([&] { *result = tx_generated::tx_add(left, right); });
}

extern "C" int txrt_sub_i64(std::int64_t left, std::int64_t right,
                             std::int64_t* result) noexcept
{
    return invoke_checked([&] { *result = tx_generated::tx_sub(left, right); });
}

extern "C" int txrt_mul_i64(std::int64_t left, std::int64_t right,
                             std::int64_t* result) noexcept
{
    return invoke_checked([&] { *result = tx_generated::tx_mul(left, right); });
}

extern "C" int txrt_div_i64(std::int64_t left, std::int64_t right,
                             std::int64_t* result) noexcept
{
    return invoke_checked([&] { *result = tx_generated::tx_div(left, right); });
}

extern "C" int txrt_neg_i64(std::int64_t value, std::int64_t* result) noexcept
{
    return invoke_checked([&] { *result = tx_generated::tx_neg(value); });
}

extern "C" int txrt_div_f64(double left, double right, double* result) noexcept
{
    return invoke_checked([&] { *result = tx_generated::tx_float_div(left, right); });
}
