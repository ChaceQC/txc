#include "backend/cpp/runtime_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/network_abi_helpers.hpp"
#include "backend/cpp/text_reference.hpp"
#include "backend/cpp/value_format.hpp"
#include "stdlib/httpx.hpp"
#include "stdlib/bytes.hpp"
#include "stdlib/typed_map.hpp"
#include "stdlib/vector.hpp"

#include <any>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

extern "C" void tx_callback_m0_bridge_serve_once_0(void*, void*, void*,
                                                     std::int64_t);
extern "C" void tx_callback_m0_bridge_serve_once_bytes_0(void*, void*, void*,
                                                           std::int64_t);
extern "C" void tx_callback_m0_bridge_serve_routes_0(void*, void*, void*,
                                                       std::int64_t);
extern "C" void tx_callback_m0_bridge_serve_routes_bytes_0(void*, void*, void*,
                                                             std::int64_t);
namespace tx_generated::network_abi
{

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;
using tx_generated::network::header_map;
thread_local std::int64_t active_http_request = 0;

const std::string& text_at(const void* value)
{
    return tx_generated::detail::text_value(value);
}

std::string_view bytes_at(const void* value)
{
    const auto& bytes = tx_generated::bytes_of(*static_cast<const std::any*>(value));
    return {reinterpret_cast<const char*>(bytes->data()), bytes->size()};
}

tx_generated::byte_value bytes_from(std::string_view value)
{
    return tx_generated::make_bytes(std::vector<std::uint8_t>(
        value.begin(), value.end()));
}

tx_generated::text_reference text_reference_of(std::string value)
{
    auto* handle = make_handle<std::string>(std::move(value));
    tx_generated::text_reference result(handle);
    tx_generated::detail::destroy_handle(handle);
    return result;
}

header_map read_headers(const std::any& value)
{
    const auto& holder = std::any_cast<const tx_generated::container_handle&>(value);
    const auto& storage = dynamic_cast<const tx_generated::map_storage<
        tx_generated::text_reference, tx_generated::text_reference>&>(*holder);
    header_map result;
    for (const auto& [name, item] : storage.values)
    {
        const auto key = tx_generated::network::lower_ascii(name.get());
        tx_generated::network::validate_header(key, item.get());
        if (!result.emplace(key, item.get()).second)
        {
            tx_generated::network::fail("invalid_header",
                "HTTP 头名称按 ASCII 大小写规范化后发生重复");
        }
    }
    return result;
}

tx_generated::container_handle write_headers(const header_map& values)
{
    using storage_type = tx_generated::map_storage<tx_generated::text_reference,
                                                   tx_generated::text_reference>;
    auto storage = std::make_shared<storage_type>();
    for (const auto& [name, value] : values)
    {
        storage->set(text_reference_of(name), text_reference_of(value));
    }
    return storage;
}

tx_generated::string_vector write_cookies(const std::vector<std::string>& values)
{
    tx_generated::string_vector result;
    result.data().values.reserve(values.size());
    for (const auto& value : values)
    {
        result.data().values.push_back(text_reference_of(value));
    }
    result.data().refresh();
    return result;
}

const tx_generated::dynamic_struct_data& struct_at(const void* value)
{
    const auto& item = *static_cast<const std::any*>(value);
    return *std::any_cast<const tx_generated::dynamic_struct&>(item).data;
}

std::int64_t resource_id(const void* value)
{
    const auto& fields = struct_at(value).fields;
    if (fields.size() != 1)
    {
        tx_generated::network::fail("invalid_argument", "网络资源结构体字段无效");
    }
    return std::any_cast<std::int64_t>(fields[0].value);
}

tx_generated::dynamic_struct make_resource(const char* type_name,
                                            const char* display_name,
                                            std::int64_t id)
{
    tx_generated::struct_fields fields(1);
    fields[0] = {"id", id};
    return tx_generated::dynamic_struct({type_name, display_name,
                                         std::move(fields)});
}

tx_generated::dynamic_struct make_response(const char* type_name,
                                            tx_generated::http_response_data value,
                                            bool binary = false)
{
    tx_generated::struct_fields fields(4);
    fields[0] = {"status", value.status};
    fields[1] = {"headers", write_headers(value.headers)};
    fields[2] = {"set_cookie", write_cookies(value.cookies)};
    fields[3] = {"body", binary ? std::any(bytes_from(value.body))
                                 : std::any(std::move(value.body))};
    return tx_generated::dynamic_struct({type_name, "response",
                                         std::move(fields)});
}

tx_generated::dynamic_struct make_request(const char* request_type,
                                           const char* connection_type,
                                           std::int64_t id, bool binary = false)
{
    auto value = tx_generated::httpx_request(id);
    tx_generated::struct_fields fields(5);
    fields[0] = {"connection", make_resource(connection_type, "connection", id)};
    fields[1] = {"method", std::move(value.method)};
    fields[2] = {"target", std::move(value.target)};
    fields[3] = {"headers", write_headers(value.headers)};
    fields[4] = {"body", binary ? std::any(bytes_from(value.body))
                                 : std::any(std::move(value.body))};
    return tx_generated::dynamic_struct({request_type, "request",
                                         std::move(fields)});
}

tx_generated::http_response_data read_response(const void* value,
                                               bool binary = false)
{
    const auto& fields = struct_at(value).fields;
    if (fields.size() != 4)
    {
        tx_generated::network::fail("invalid_argument", "HTTP 响应结构体字段无效");
    }
    tx_generated::http_response_data result;
    result.status = std::any_cast<std::int64_t>(fields[0].value);
    result.headers = read_headers(fields[1].value);
    for (const auto& item : std::any_cast<const tx_generated::string_vector&>(
             fields[2].value).data().values)
    {
        result.cookies.push_back(item.get());
    }
    if (binary)
    {
        const auto& bytes = tx_generated::bytes_of(fields[3].value);
        result.body.assign(reinterpret_cast<const char*>(bytes->data()), bytes->size());
    }
    else
    {
        result.body = std::any_cast<const std::string&>(fields[3].value);
    }
    return result;
}

void* client_response(std::string_view method, std::string_view url,
                      const header_map& headers, std::string_view body,
                      std::int64_t timeout, const char* type_name,
                      bool binary, bool http2)
{
    const auto id = tx_generated::httpx_client_send(method, url, headers,
                                                     body, timeout, binary, http2);
    try
    {
        auto value = tx_generated::httpx_response(id);
        auto* result = make_handle<std::any>(make_response(type_name,
                                                            std::move(value), binary));
        tx_generated::httpx_release_response(id);
        return result;
    }
    catch (...)
    {
        tx_generated::httpx_release_response(id);
        throw;
    }
}

} // namespace tx_generated::network_abi

using namespace tx_generated::network_abi;

extern "C" int txrt_httpx_send(const void* method, const void* url,
    const void* headers, const void* body, std::int64_t timeout,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = client_response(text_at(method), text_at(url),
            read_headers(*static_cast<const std::any*>(headers)), text_at(body),
            timeout, type_name);
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_get(const void* url, std::int64_t timeout,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = client_response("GET", text_at(url), {}, "", timeout, type_name);
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_post(const void* url, const void* body,
    std::int64_t timeout, const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = client_response("POST", text_at(url), {}, text_at(body),
                                  timeout, type_name);
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_send_bytes(const void* method, const void* url,
    const void* headers, const void* body, std::int64_t timeout,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = client_response(text_at(method), text_at(url),
            read_headers(*static_cast<const std::any*>(headers)), bytes_at(body),
            timeout, type_name, true);
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_get_bytes(const void* url, std::int64_t timeout,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = client_response("GET", text_at(url), {}, "", timeout,
                                  type_name, true);
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_post_bytes(const void* url, const void* body,
    std::int64_t timeout, const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = client_response("POST", text_at(url), {}, bytes_at(body),
                                  timeout, type_name, true);
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_listen(const void* host, std::int64_t port,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto id = tx_generated::httpx_listen(text_at(host), port);
        *result = make_handle<std::any>(make_resource(type_name, "listener", id));
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_listen_with_limit(const void* host,
    std::int64_t port, std::int64_t max_connections,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto id = tx_generated::httpx_listen_with_limit(text_at(host),
            port, max_connections);
        *result = make_handle<std::any>(make_resource(type_name, "listener", id));
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_accept(const void* server, std::int64_t timeout,
    const char* request_type, const char* connection_type,
    void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto id = tx_generated::httpx_accept(resource_id(server), timeout);
        active_http_request = id;
        try
        {
            *result = make_handle<std::any>(make_request(request_type,
                                                          connection_type, id));
        }
        catch (...)
        {
            tx_generated::httpx_close_connection(id);
            throw;
        }
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_respond(const void* peer,
    const void* value) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::httpx_respond(resource_id(peer), read_response(value));
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_accept_bytes(const void* server,
    std::int64_t timeout, const char* request_type,
    const char* connection_type, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto id = tx_generated::httpx_accept(resource_id(server), timeout, true);
        active_http_request = id;
        try
        {
            *result = make_handle<std::any>(make_request(request_type,
                                                          connection_type, id, true));
        }
        catch (...)
        {
            tx_generated::httpx_close_connection(id);
            throw;
        }
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_respond_bytes(const void* peer,
    const void* value) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::httpx_respond(resource_id(peer), read_response(value, true), true);
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_serve_once(const void* server, void* handler,
    std::int64_t timeout, const char*, const char*) noexcept
{
    return invoke_checked([&]
    {
        active_http_request = 0;
        auto* owned = make_handle<std::any>(*static_cast<const std::any*>(server));
        auto* owned_handler = make_handle<std::any>(
            *static_cast<const std::any*>(handler));
        tx_callback_m0_bridge_serve_once_0(nullptr, owned, owned_handler, timeout);
        if (txrt_error_status() != 0 && active_http_request != 0)
        {
            tx_generated::httpx_close_connection(active_http_request);
        }
        active_http_request = 0;
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_serve_once_bytes(const void* server, void* handler,
    std::int64_t timeout, const char*, const char*) noexcept
{
    return invoke_checked([&]
    {
        active_http_request = 0;
        auto* owned = make_handle<std::any>(*static_cast<const std::any*>(server));
        auto* owned_handler = make_handle<std::any>(
            *static_cast<const std::any*>(handler));
        tx_callback_m0_bridge_serve_once_bytes_0(nullptr, owned,
                                                   owned_handler, timeout);
        if (txrt_error_status() != 0 && active_http_request != 0)
        {
            tx_generated::httpx_close_connection(active_http_request);
        }
        active_http_request = 0;
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_serve_routes(const void* server, void* routes,
    std::int64_t timeout, const char*, const char*) noexcept
{
    return invoke_checked([&]
    {
        active_http_request = 0;
        auto* owned = make_handle<std::any>(*static_cast<const std::any*>(server));
        auto* owned_routes = make_handle<std::any>(
            *static_cast<const std::any*>(routes));
        tx_callback_m0_bridge_serve_routes_0(nullptr, owned, owned_routes,
                                              timeout);
        if (txrt_error_status() != 0 && active_http_request != 0)
        {
            tx_generated::httpx_close_connection(active_http_request);
        }
        active_http_request = 0;
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_serve_routes_bytes(const void* server, void* routes,
    std::int64_t timeout, const char*, const char*) noexcept
{
    return invoke_checked([&]
    {
        active_http_request = 0;
        auto* owned = make_handle<std::any>(*static_cast<const std::any*>(server));
        auto* owned_routes = make_handle<std::any>(
            *static_cast<const std::any*>(routes));
        tx_callback_m0_bridge_serve_routes_bytes_0(nullptr, owned,
                                                    owned_routes, timeout);
        if (txrt_error_status() != 0 && active_http_request != 0)
        {
            tx_generated::httpx_close_connection(active_http_request);
        }
        active_http_request = 0;
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_close_listener(const void* server) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::httpx_close_listener(resource_id(server));
    }, tx::error_kind::io);
}

extern "C" int txrt_httpx_close_connection(const void* peer) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::httpx_close_connection(resource_id(peer));
    }, tx::error_kind::io);
}
