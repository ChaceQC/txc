#include "backend/cpp/external_direct_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/bytes.hpp"
#include "stdlib/file_extra.hpp"
#include "stdlib/stdlib.hpp"

#include <any>
#include <string>

namespace
{

const std::string& text_value(const void* value)
{
    return tx_generated::detail::text_value(value);
}

} // namespace

using tx_generated::detail::invoke_checked;

extern "C" int txrt_file_read_text(const void* path, const void* encoding,
                                     void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::detail::make_handle<std::string>(tx_generated::tx_fn_read_text(
            text_value(path), text_value(encoding)));
    }, tx::error_kind::io);
}

extern "C" int txrt_file_write_text(const void* path, const void* text,
                                      const void* encoding) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::tx_fn_write_text(text_value(path), text_value(text),
                                       text_value(encoding));
    }, tx::error_kind::io);
}

extern "C" int txrt_file_append_text(const void* path, const void* text,
                                       const void* encoding) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::tx_fn_append_text(text_value(path), text_value(text),
                                        text_value(encoding));
    }, tx::error_kind::io);
}

extern "C" int txrt_file_read_bytes(const void* path, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::detail::make_handle<std::any>(
            tx_generated::file_read_bytes(text_value(path)));
    }, tx::error_kind::io);
}

extern "C" int txrt_file_write_bytes(const void* path,
                                      const void* data) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::file_write_bytes(text_value(path),
            tx_generated::bytes_of(*static_cast<const std::any*>(data)));
    }, tx::error_kind::io);
}

extern "C" int txrt_file_atomic_write_bytes(const void* path,
                                             const void* data) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::file_atomic_write_bytes(text_value(path),
            tx_generated::bytes_of(*static_cast<const std::any*>(data)));
    }, tx::error_kind::io);
}

extern "C" int txrt_file_atomic_write_text(const void* path, const void* text,
                                            const void* encoding) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::file_atomic_write_text(text_value(path), text_value(text),
            text_value(encoding));
    }, tx::error_kind::io);
}
