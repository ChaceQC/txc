#include "backend/cpp/parse_abi.hpp"
#include "backend/cpp/error_result.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/stdlib.hpp"

#include <any>
#include <new>
#include <string>

namespace
{

const std::string& text_value(const void* value)
{
    return tx_generated::detail::text_value(value);
}

template<class value_type, class operation>
int file_result(const char* result_type, const char* error_type,
                void** result, operation&& run) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        tx_generated::operation_result<value_type> value;
        try
        {
            value.value = run();
            value.ok = true;
        }
        catch (const std::bad_alloc&)
        {
            throw;
        }
        catch (const std::exception& error)
        {
            value.error = {tx::error_kind::io, "operation_failed", error.what()};
        }
        *result = tx_generated::detail::make_handle<std::any>(
            tx_generated::detail::make_result_value(result_type, error_type, std::move(value)));
    });
}

} // namespace

extern "C" int txrt_file_try_read_text(const void* path, const void* encoding,
    const char* result_type, const char* error_type, void** result) noexcept
{
    return file_result<std::string>(result_type, error_type, result, [&]
    {
        return tx_generated::tx_fn_read_text(text_value(path), text_value(encoding));
    });
}

extern "C" int txrt_file_try_write_text(const void* path, const void* text,
    const void* encoding, const char* result_type, const char* error_type,
    void** result) noexcept
{
    return file_result<bool>(result_type, error_type, result, [&]
    {
        tx_generated::tx_fn_write_text(text_value(path), text_value(text), text_value(encoding));
        return true;
    });
}

extern "C" int txrt_file_try_append_text(const void* path, const void* text,
    const void* encoding, const char* result_type, const char* error_type,
    void** result) noexcept
{
    return file_result<bool>(result_type, error_type, result, [&]
    {
        tx_generated::tx_fn_append_text(text_value(path), text_value(text), text_value(encoding));
        return true;
    });
}
