#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/value_format.hpp"
#include "stdlib/file_stream.hpp"

#include <any>
#include <string>
#include <utility>

namespace
{

const std::string& text_argument(const void* value)
{
    return *static_cast<const std::string*>(value);
}

const tx_generated::binary_stream& binary_argument(const void* value)
{
    const auto& result = std::any_cast<const tx_generated::binary_stream&>(
        *static_cast<const std::any*>(value));
    if (!result)
    {
        throw tx_generated::runtime_failure({tx::error_kind::io,
            "closed_stream", "二进制文件流未打开或已关闭"});
    }
    return result;
}

const tx_generated::text_stream& stream_argument(const void* value)
{
    const auto& result = std::any_cast<const tx_generated::text_stream&>(
        *static_cast<const std::any*>(value));
    if (!result)
    {
        throw tx_generated::runtime_failure({tx::error_kind::io,
            "closed_stream", "文本文件流未打开或已关闭"});
    }
    return result;
}

tx_generated::dynamic_struct byte_chunk_struct(
    const char* type_name, tx_generated::byte_chunk_value value)
{
    tx_generated::struct_fields fields(2);
    fields[0] = {"data", std::move(value.data)};
    fields[1] = {"eof", value.eof};
    return tx_generated::dynamic_struct(tx_generated::dynamic_struct_data{
        type_name, "byte_chunk", std::move(fields)});
}

tx_generated::dynamic_struct text_chunk_struct(
    const char* type_name, tx_generated::text_chunk_value value)
{
    tx_generated::struct_fields fields(2);
    fields[0] = {"text", std::move(value.text)};
    fields[1] = {"eof", value.eof};
    return tx_generated::dynamic_struct(tx_generated::dynamic_struct_data{
        type_name, "text_chunk", std::move(fields)});
}

tx_generated::dynamic_struct line_result_struct(
    const char* type_name, tx_generated::line_result_value value)
{
    tx_generated::struct_fields fields(2);
    fields[0] = {"has_line", value.has_line};
    fields[1] = {"line", std::move(value.line)};
    return tx_generated::dynamic_struct(tx_generated::dynamic_struct_data{
        type_name, "line_result", std::move(fields)});
}

} // namespace

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

extern "C" int txrt_file_stream_open_binary(const void* path, const void* mode,
                                               void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::open_binary_stream(
            text_argument(path), text_argument(mode)));
    }, tx::error_kind::io);
}

extern "C" int txrt_file_stream_open_binary_shared(const void* path,
                                                     const void* mode,
                                                     void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::open_binary_stream(
            text_argument(path), text_argument(mode), true));
    }, tx::error_kind::io);
}

extern "C" int txrt_file_stream_read_bytes(const void* source, std::int64_t size,
                                             const char* type_name,
                                             void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(byte_chunk_struct(type_name,
            tx_generated::stream_read_bytes(binary_argument(source), size)));
    }, tx::error_kind::io);
}

extern "C" int txrt_file_stream_read_all_bytes(const void* source,
                                                 void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::stream_read_all_bytes(
            binary_argument(source)));
    }, tx::error_kind::io);
}

extern "C" int txrt_file_stream_write_bytes(const void* target,
                                               const void* data) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::stream_write_bytes(binary_argument(target),
            tx_generated::bytes_of(*static_cast<const std::any*>(data)));
    }, tx::error_kind::io);
}

extern "C" int txrt_file_stream_tell(const void* source,
                                       std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::stream_tell(binary_argument(source));
    }, tx::error_kind::io);
}

extern "C" int txrt_file_stream_seek(const void* source, std::int64_t offset,
                                       const void* origin,
                                       std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::stream_seek(binary_argument(source), offset,
            text_argument(origin));
    }, tx::error_kind::io);
}

extern "C" int txrt_file_stream_flush_binary(const void* target) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::stream_flush(binary_argument(target));
    }, tx::error_kind::io);
}

extern "C" int txrt_file_stream_sync_binary(const void* target) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::stream_sync(binary_argument(target));
    }, tx::error_kind::io);
}

extern "C" int txrt_file_stream_lock(const void* target,
                                      const void* mode) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::stream_lock(binary_argument(target), text_argument(mode));
    }, tx::error_kind::io);
}

extern "C" int txrt_file_stream_try_lock(const void* target,
                                          const void* mode,
                                          bool* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::stream_try_lock(binary_argument(target),
            text_argument(mode));
    }, tx::error_kind::io);
}

extern "C" int txrt_file_stream_unlock(const void* target) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::stream_unlock(binary_argument(target));
    }, tx::error_kind::io);
}

extern "C" int txrt_file_stream_close_binary(const void* target) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::stream_close(binary_argument(target));
    }, tx::error_kind::io);
}

extern "C" int txrt_file_stream_open_text(const void* path, const void* mode,
                                            const void* encoding,
                                            void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::open_text_stream(
            text_argument(path), text_argument(mode), text_argument(encoding)));
    }, tx::error_kind::io);
}

extern "C" int txrt_file_stream_read_chars(const void* source,
                                             std::int64_t count,
                                             const char* type_name,
                                             void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(text_chunk_struct(type_name,
            tx_generated::stream_read_chars(stream_argument(source), count)));
    }, tx::error_kind::io);
}

extern "C" int txrt_file_stream_read_line(const void* source,
                                            std::int64_t max_bytes,
                                            const char* type_name,
                                            void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(line_result_struct(type_name,
            tx_generated::stream_read_line(stream_argument(source), max_bytes)));
    }, tx::error_kind::io);
}

extern "C" int txrt_file_stream_write_text(const void* target,
                                             const void* text) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::stream_write_text(stream_argument(target),
            text_argument(text));
    }, tx::error_kind::io);
}

extern "C" int txrt_file_stream_write_line(const void* target,
                                             const void* text) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::stream_write_line(stream_argument(target),
            text_argument(text));
    }, tx::error_kind::io);
}

extern "C" int txrt_file_stream_flush_text(const void* target) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::stream_flush(stream_argument(target));
    }, tx::error_kind::io);
}

extern "C" int txrt_file_stream_sync_text(const void* target) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::stream_sync(stream_argument(target));
    }, tx::error_kind::io);
}

extern "C" int txrt_file_stream_close_text(const void* target) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::stream_close(stream_argument(target));
    }, tx::error_kind::io);
}
