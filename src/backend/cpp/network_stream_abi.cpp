#include "backend/cpp/network_abi_helpers.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/httpx.hpp"
#include "stdlib/http2_server.hpp"

#include <any>
#include <cstdint>
#include <utility>

namespace
{

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;
using namespace tx_generated::network_abi;

const tx_generated::binary_stream& binary_argument(const void* value)
{
    return std::any_cast<const tx_generated::binary_stream&>(
        *static_cast<const std::any*>(value));
}

tx_generated::dynamic_struct stream_response_struct(
    const char* type_name, tx_generated::http_response_data value)
{
    tx_generated::struct_fields fields(4);
    fields[0] = {"status", value.status};
    fields[1] = {"headers", write_headers(value.headers)};
    fields[2] = {"set_cookie", write_cookies(value.cookies)};
    fields[3] = {"body_length", value.body_length};
    return tx_generated::dynamic_struct({type_name, "stream_response",
                                         std::move(fields)});
}

tx_generated::dynamic_struct stream_request_struct(
    const char* request_type, const char* connection_type, std::int64_t id)
{
    auto value = tx_generated::httpx_request(id);
    tx_generated::struct_fields fields(5);
    fields[0] = {"connection", make_resource(connection_type, "connection", id)};
    fields[1] = {"method", std::move(value.method)};
    fields[2] = {"target", std::move(value.target)};
    fields[3] = {"headers", write_headers(value.headers)};
    fields[4] = {"body_length", value.body_length};
    return tx_generated::dynamic_struct({request_type, "stream_request",
                                         std::move(fields)});
}

std::vector<std::string> cookies_argument(const void* value)
{
    const auto& values = std::any_cast<const tx_generated::string_vector&>(
        *static_cast<const std::any*>(value)).data().values;
    std::vector<std::string> result;
    result.reserve(values.size());
    for (const auto& item : values)
    {
        result.push_back(item.get());
    }
    return result;
}

} // namespace

extern "C" int txrt_httpx_listen_h2c(const void* host, std::int64_t port,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto id = tx_generated::http2::listen_h2c(text_at(host), port);
        *result = make_handle<std::any>(make_resource(type_name, "listener", id));
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_listen_h2_tls(const void* host,
    std::int64_t port, const void* cert_pem, const void* key_pem,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto id = tx_generated::http2::listen_h2_tls(text_at(host), port,
            text_at(cert_pem), text_at(key_pem));
        *result = make_handle<std::any>(make_resource(type_name, "listener", id));
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_send_stream(const void* method, const void* url,
    const void* headers, const void* source, std::int64_t source_length,
    const void* destination, std::int64_t max_response_bytes,
    std::int64_t timeout, const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        auto value = tx_generated::httpx_client_stream(text_at(method), text_at(url),
            read_headers(*static_cast<const std::any*>(headers)),
            binary_argument(source), source_length, binary_argument(destination),
            max_response_bytes, timeout);
        *result = make_handle<std::any>(stream_response_struct(type_name,
                                                                std::move(value)));
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_get_stream(const void* url,
    const void* destination, std::int64_t max_response_bytes,
    std::int64_t timeout, const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        auto value = tx_generated::httpx_client_stream("GET", text_at(url), {},
            {}, 0, binary_argument(destination), max_response_bytes, timeout);
        *result = make_handle<std::any>(stream_response_struct(type_name,
                                                                std::move(value)));
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_send_http2_stream(const void* method,
    const void* url, const void* headers, const void* source,
    std::int64_t source_length, const void* destination,
    std::int64_t max_response_bytes, std::int64_t timeout,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        auto value = tx_generated::httpx_client_stream(text_at(method), text_at(url),
            read_headers(*static_cast<const std::any*>(headers)),
            binary_argument(source), source_length, binary_argument(destination),
            max_response_bytes, timeout, true);
        *result = make_handle<std::any>(stream_response_struct(type_name,
                                                                std::move(value)));
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_get_http2_stream(const void* url,
    const void* destination, std::int64_t max_response_bytes,
    std::int64_t timeout, const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        auto value = tx_generated::httpx_client_stream("GET", text_at(url), {},
            {}, 0, binary_argument(destination), max_response_bytes, timeout,
            true);
        *result = make_handle<std::any>(stream_response_struct(type_name,
                                                                std::move(value)));
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_accept_stream(const void* server,
    const void* destination, std::int64_t max_request_bytes,
    std::int64_t timeout, const char* request_type,
    const char* connection_type, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto id = tx_generated::httpx_accept_stream(resource_id(server),
            binary_argument(destination), max_request_bytes, timeout);
        try
        {
            *result = make_handle<std::any>(stream_request_struct(request_type,
                                                                    connection_type, id));
        }
        catch (...)
        {
            tx_generated::httpx_close_connection(id);
            throw;
        }
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_respond_stream(const void* peer,
    std::int64_t status, const void* headers, const void* set_cookie,
    const void* source, std::int64_t body_length) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::http_response_data value;
        value.status = status;
        value.headers = read_headers(*static_cast<const std::any*>(headers));
        value.cookies = cookies_argument(set_cookie);
        tx_generated::httpx_respond_stream(resource_id(peer), value,
                                            binary_argument(source), body_length);
    }, tx::error_kind::io);
}
