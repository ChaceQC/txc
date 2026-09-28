#include "backend/cpp/network_abi_helpers.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/httpx_client_session.hpp"

#include <any>
#include <cstdint>
#include <utility>

namespace
{

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;
using namespace tx_generated::network_abi;

tx_generated::dynamic_struct make_head(const char* type_name,
                                       tx_generated::http_response_data value)
{
    tx_generated::struct_fields fields(3);
    fields[0] = {"status", value.status};
    fields[1] = {"headers", write_headers(value.headers)};
    fields[2] = {"set_cookie", write_cookies(value.cookies)};
    return tx_generated::dynamic_struct({type_name, "response_head",
                                         std::move(fields)});
}

tx_generated::dynamic_struct make_chunk(const char* type_name,
                                        tx_generated::httpx_session::response_chunk value)
{
    tx_generated::struct_fields fields(2);
    fields[0] = {"data", bytes_from(value.data)};
    fields[1] = {"eof", value.eof};
    return tx_generated::dynamic_struct({type_name, "response_chunk",
                                         std::move(fields)});
}

} // namespace

extern "C" int txrt_httpx_open_session(const void* proxy_url,
    std::int64_t max_connections, bool decompress,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto id = tx_generated::httpx_session::open(text_at(proxy_url),
            max_connections, decompress);
        *result = make_handle<std::any>(make_resource(type_name,
            "client_session", id));
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_open_secure_session(const void* proxy_url,
    std::int64_t max_connections, bool decompress, const void* anchors,
    bool include_system, const void* package, const void* password,
    bool allow_http2, const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto& roots = std::any_cast<const tx_generated::bytes_vector&>(
            *static_cast<const std::any*>(anchors));
        const auto& pfx = std::any_cast<const tx_generated::byte_value&>(
            *static_cast<const std::any*>(package));
        const auto& credential = std::any_cast<const tx_generated::secret::handle&>(
            *static_cast<const std::any*>(password));
        const auto id = tx_generated::httpx_session::open_secure(
            text_at(proxy_url), max_connections, decompress,
            roots, include_system, pfx, credential, allow_http2);
        *result = make_handle<std::any>(make_resource(type_name,
            "client_session", id));
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_begin_request(const void* session,
    const void* method, const void* url, const void* headers,
    std::int64_t body_length, std::int64_t max_response_bytes,
    std::int64_t timeout_ms, bool require_http2,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto id = tx_generated::httpx_session::begin(resource_id(session),
            text_at(method), text_at(url),
            read_headers(*static_cast<const std::any*>(headers)), body_length,
            max_response_bytes, timeout_ms, require_http2);
        *result = make_handle<std::any>(make_resource(type_name,
            "client_request", id));
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_write_request(const void* peer,
    const void* data, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::httpx_session::write(resource_id(peer),
            bytes_at(data));
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_finish_request(const void* peer,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        auto value = tx_generated::httpx_session::finish(resource_id(peer));
        *result = make_handle<std::any>(make_head(type_name, std::move(value)));
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_read_response_chunk(const void* peer,
    std::int64_t max_bytes, const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        auto value = tx_generated::httpx_session::read(resource_id(peer),
            max_bytes);
        *result = make_handle<std::any>(make_chunk(type_name, std::move(value)));
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_close_session(const void* session) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::httpx_session::close(resource_id(session));
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_close_request(const void* peer) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::httpx_session::close_request(resource_id(peer));
    }, tx::error_kind::io);
}
