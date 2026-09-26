#include "backend/cpp/network_abi_helpers.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"

#include <any>
#include <cstdint>

using tx_generated::detail::invoke_checked;
using namespace tx_generated::network_abi;

extern "C" int txrt_httpx_send_http2(const void* method, const void* url,
    const void* headers, const void* body, std::int64_t timeout,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = client_response(text_at(method), text_at(url),
            read_headers(*static_cast<const std::any*>(headers)),
            text_at(body), timeout, type_name, false, true);
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_get_http2(const void* url, std::int64_t timeout,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = client_response("GET", text_at(url), {}, "", timeout,
                                  type_name, false, true);
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_post_http2(const void* url, const void* body,
    std::int64_t timeout, const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = client_response("POST", text_at(url), {}, text_at(body),
                                  timeout, type_name, false, true);
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_send_http2_bytes(const void* method,
    const void* url, const void* headers, const void* body,
    std::int64_t timeout, const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = client_response(text_at(method), text_at(url),
            read_headers(*static_cast<const std::any*>(headers)),
            bytes_at(body), timeout, type_name, true, true);
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_get_http2_bytes(const void* url,
    std::int64_t timeout, const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = client_response("GET", text_at(url), {}, "", timeout,
                                  type_name, true, true);
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_post_http2_bytes(const void* url,
    const void* body, std::int64_t timeout, const char* type_name,
    void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = client_response("POST", text_at(url), {}, bytes_at(body),
                                  timeout, type_name, true, true);
    }, tx::error_kind::io);
}
