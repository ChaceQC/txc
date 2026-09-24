#include "backend/cpp/external_direct_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/stdlib.hpp"

#include <string>

namespace
{

const std::string& text_value(const void* value)
{
    return *static_cast<const std::string*>(value);
}

} // namespace

using tx_generated::detail::invoke_checked;

extern "C" int txrt_io_write(const void* text) noexcept
{
    return invoke_checked([&] { tx_generated::tx_fn_write(text_value(text)); });
}

extern "C" int txrt_io_write_line(const void* text) noexcept
{
    return invoke_checked([&] { tx_generated::tx_fn_write_line(text_value(text)); });
}

extern "C" int txrt_io_write_error(const void* text) noexcept
{
    return invoke_checked([&] { tx_generated::tx_fn_write_error(text_value(text)); });
}

extern "C" int txrt_io_flush() noexcept
{
    return invoke_checked([] { tx_generated::tx_fn_flush(); });
}

extern "C" int txrt_time_unix_millis(std::int64_t* result) noexcept
{
    return invoke_checked([&] { *result = tx_generated::tx_fn_unix_millis(); });
}

extern "C" int txrt_time_monotonic_millis(std::int64_t* result) noexcept
{
    return invoke_checked([&] { *result = tx_generated::tx_fn_monotonic_millis(); });
}

extern "C" int txrt_time_sleep_millis(std::int64_t duration) noexcept
{
    return invoke_checked([&] { tx_generated::tx_fn_sleep_millis(duration); });
}
