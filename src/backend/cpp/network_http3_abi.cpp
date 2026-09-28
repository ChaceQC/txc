#include "backend/cpp/network_abi_helpers.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/http3_client.hpp"
#include "stdlib/http3_server.hpp"
#include "stdlib/httpx_negotiation.hpp"

#include <any>

namespace
{

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;
using namespace tx_generated::network_abi;

void* send(std::string_view method, std::string_view url,
           const tx_generated::network::header_map& headers,
           std::string_view body, std::int64_t timeout,
           const char* type_name, bool binary,
           const std::vector<tx_generated::byte_value>& anchors = {},
           const tx_generated::cancel_token* token = nullptr)
{
    auto value = tx_generated::http3::client_send(method, url, headers,
        body, timeout, binary, anchors, token);
    return make_handle<std::any>(make_response(type_name,
                                               std::move(value), binary));
}

const std::vector<tx_generated::byte_value>& trust_at(const void* value)
{
    return std::any_cast<const tx_generated::bytes_vector&>(
        *static_cast<const std::any*>(value)).data().values;
}

const tx_generated::cancel_token& token_at(const void* value)
{
    return std::any_cast<const tx_generated::cancel_token&>(
        *static_cast<const std::any*>(value));
}

tx_generated::dynamic_struct negotiated_value(const char* type_name,
    tx_generated::http_response_data value, bool binary)
{
    tx_generated::struct_fields fields(5);
    fields[0] = {"protocol", std::move(value.protocol)};
    fields[1] = {"status", value.status};
    fields[2] = {"headers", write_headers(value.headers)};
    fields[3] = {"set_cookie", write_cookies(value.cookies)};
    fields[4] = {"body", binary
        ? std::any(bytes_from(value.body)) : std::any(std::move(value.body))};
    return tx_generated::dynamic_struct({type_name,
        binary ? "negotiated_binary_response" : "negotiated_response",
        std::move(fields)});
}

} // namespace

extern "C" int txrt_httpx_send_http3(const void* method, const void* url,
    const void* headers, const void* body, std::int64_t timeout,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = send(text_at(method), text_at(url),
            read_headers(*static_cast<const std::any*>(headers)),
            text_at(body), timeout, type_name, false);
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_get_http3(const void* url,
    std::int64_t timeout, const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = send("GET", text_at(url), {}, "", timeout, type_name, false);
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_post_http3(const void* url, const void* body,
    std::int64_t timeout, const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = send("POST", text_at(url), {}, text_at(body), timeout,
                       type_name, false);
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_send_http3_bytes(const void* method,
    const void* url, const void* headers, const void* body,
    std::int64_t timeout, const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = send(text_at(method), text_at(url),
            read_headers(*static_cast<const std::any*>(headers)),
            bytes_at(body), timeout, type_name, true);
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_get_http3_bytes(const void* url,
    std::int64_t timeout, const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = send("GET", text_at(url), {}, "", timeout, type_name, true);
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_post_http3_bytes(const void* url,
    const void* body, std::int64_t timeout, const char* type_name,
    void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = send("POST", text_at(url), {}, bytes_at(body), timeout,
                       type_name, true);
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_listen_h3(const void* host, std::int64_t port,
    const void* package, const void* password,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto& certificate = std::any_cast<const tx_generated::byte_value&>(
            *static_cast<const std::any*>(package));
        const auto& secret = std::any_cast<const tx_generated::secret::handle&>(
            *static_cast<const std::any*>(password));
        const auto id = tx_generated::http3::listen(text_at(host), port,
                                                     certificate, secret);
        *result = make_handle<std::any>(make_resource(type_name, "listener", id));
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_send_http3_with_trust(const void* method,
    const void* url, const void* headers, const void* body,
    const void* anchors, std::int64_t timeout,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = send(text_at(method), text_at(url),
            read_headers(*static_cast<const std::any*>(headers)),
            text_at(body), timeout, type_name, false, trust_at(anchors));
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_send_http3_with_trust_bytes(const void* method,
    const void* url, const void* headers, const void* body,
    const void* anchors, std::int64_t timeout,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = send(text_at(method), text_at(url),
            read_headers(*static_cast<const std::any*>(headers)),
            bytes_at(body), timeout, type_name, true, trust_at(anchors));
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_send_negotiated(const void* method,
    const void* url, const void* headers, const void* body,
    std::int64_t timeout, const void* policy,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        auto value = tx_generated::httpx_send_negotiated(text_at(method),
            text_at(url), read_headers(*static_cast<const std::any*>(headers)),
            text_at(body), timeout, text_at(policy), false);
        *result = make_handle<std::any>(negotiated_value(type_name,
                                                          std::move(value), false));
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_send_negotiated_bytes(const void* method,
    const void* url, const void* headers, const void* body,
    std::int64_t timeout, const void* policy,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        auto value = tx_generated::httpx_send_negotiated(text_at(method),
            text_at(url), read_headers(*static_cast<const std::any*>(headers)),
            bytes_at(body), timeout, text_at(policy), true);
        *result = make_handle<std::any>(negotiated_value(type_name,
                                                          std::move(value), true));
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_send_http3_controlled(const void* method,
    const void* url, const void* headers, const void* body,
    const void* anchors, std::int64_t timeout, const void* token,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = send(text_at(method), text_at(url),
            read_headers(*static_cast<const std::any*>(headers)),
            text_at(body), timeout, type_name, false, trust_at(anchors),
            &token_at(token));
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_send_http3_controlled_bytes(const void* method,
    const void* url, const void* headers, const void* body,
    const void* anchors, std::int64_t timeout, const void* token,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = send(text_at(method), text_at(url),
            read_headers(*static_cast<const std::any*>(headers)),
            bytes_at(body), timeout, type_name, true, trust_at(anchors),
            &token_at(token));
    }, tx::error_kind::io);
}
