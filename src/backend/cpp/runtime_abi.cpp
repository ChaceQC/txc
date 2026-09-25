#include "backend/cpp/runtime_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/cycle_gc.hpp"

#include "backend/cpp/runtime.hpp"

#include <any>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <unordered_map>

namespace tx_generated::detail
{

thread_local char last_error[256]{};
namespace
{
thread_local std::unordered_map<void*, handle_kind> live_handles;
thread_local bool cleaning_handles = false;
}

void register_handle(void* value, handle_kind kind)
{
    live_handles.emplace(value, kind);
}

void unregister_handle(void* value) noexcept
{
    live_handles.erase(value);
}

bool cleanup_in_progress() noexcept
{
    return cleaning_handles;
}

void cleanup_live_handles() noexcept
{
    if (cleaning_handles)
    {
        return;
    }
    cleaning_handles = true;
    while (!live_handles.empty())
    {
        const auto found = live_handles.begin();
        void* value = found->first;
        const auto kind = found->second;
        live_handles.erase(found);
        if (kind == handle_kind::text)
        {
            delete static_cast<std::string*>(value);
        }
        else
        {
            delete static_cast<std::any*>(value);
        }
    }
    try
    {
        tx_generated::collect_cycles();
    }
    catch (...)
    {
        // 运行时已在退出，回收器分配失败时由操作系统回收进程内存。
    }
    cleaning_handles = false;
}

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
        if (tx_generated::detail::cleanup_in_progress())
        {
            std::_Exit(1);
        }
        tx_generated::detail::cleanup_live_handles();
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
    const auto gc_status = invoke_checked([]
    {
        tx_generated::collect_cycles();
    });
    txrt_require_success(gc_status);
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
    return invoke_checked([&]
    {
        *result = tx_generated::detail::make_handle<std::string>(bytes, length);
    });
}

extern "C" int txrt_str_clone(const void* value, void** result) noexcept
{
    return invoke_checked([&] {
        *result = tx_generated::detail::make_handle<std::string>(
            *static_cast<const std::string*>(value));
    });
}

extern "C" void txrt_str_release(void* value) noexcept
{
    tx_generated::detail::destroy_handle(static_cast<std::string*>(value));
}

extern "C" int txrt_str_concat(const void* left, const void* right,
                                void** result) noexcept
{
    return invoke_checked([&] {
        *result = tx_generated::detail::make_handle<std::string>(
            *static_cast<const std::string*>(left) +
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
        *result = tx_generated::detail::make_handle<std::string>(prompt
            ? tx_generated::tx_input(*static_cast<const std::string*>(prompt))
            : tx_generated::tx_input());
    });
}

extern "C" int txrt_input_or_none(const void* prompt, void** result) noexcept
{
    return invoke_checked([&] {
        *result = tx_generated::detail::make_handle<std::any>(prompt
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
        *result = tx_generated::detail::make_handle<std::string>(
            tx_generated::tx_int_to_string(value));
    });
}

extern "C" int txrt_float_to_str(double value, void** result) noexcept
{
    return invoke_checked([&] {
        *result = tx_generated::detail::make_handle<std::string>(
            tx_generated::tx_float_to_string(value));
    });
}

extern "C" int txrt_bool_to_str(bool value, void** result) noexcept
{
    return invoke_checked([&] {
        *result = tx_generated::detail::make_handle<std::string>(
            tx_generated::tx_bool_to_string(value));
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
