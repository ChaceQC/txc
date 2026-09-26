#include "backend/cpp/network_abi_helpers.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/ws.hpp"

#include <any>
#include <cstdint>
#include <utility>

extern "C" bool tx_callback_m0_bridge_reply_once_0(void*, void*, std::int64_t);
extern "C" bool tx_callback_m0_bridge_reply_once_binary_0(void*, void*, std::int64_t);

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;
using namespace tx_generated::network_abi;

extern "C" int txrt_ws_connect(const void* url, std::int64_t timeout,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto id = tx_generated::ws_connect(text_at(url), timeout);
        *result = make_handle<std::any>(make_resource(type_name, "connection", id));
    }, tx::error_kind::io);
}

extern "C" int txrt_ws_listen(const void* host, std::int64_t port,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto id = tx_generated::ws_listen(text_at(host), port);
        *result = make_handle<std::any>(make_resource(type_name, "listener", id));
    }, tx::error_kind::io);
}

extern "C" int txrt_ws_accept(const void* server, std::int64_t timeout,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto id = tx_generated::ws_accept(resource_id(server), timeout);
        *result = make_handle<std::any>(make_resource(type_name, "connection", id));
    }, tx::error_kind::io);
}

extern "C" int txrt_ws_send_text(const void* peer, const void* text) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::ws_send_text(resource_id(peer), text_at(text));
    }, tx::error_kind::io);
}

extern "C" int txrt_ws_send_binary(const void* peer,
    const void* data) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::ws_send_binary(resource_id(peer), bytes_at(data));
    }, tx::error_kind::io);
}

extern "C" int txrt_ws_send_binary_stream(const void* peer,
    const void* source, std::int64_t length) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::ws_send_binary_stream(resource_id(peer),
            std::any_cast<const tx_generated::binary_stream&>(
                *static_cast<const std::any*>(source)), length);
    }, tx::error_kind::io);
}

extern "C" int txrt_ws_receive(const void* peer, std::int64_t timeout,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        auto value = tx_generated::ws_receive(resource_id(peer), timeout);
        tx_generated::struct_fields fields(2);
        fields[0] = {"open", value.open};
        fields[1] = {"text", std::move(value.text)};
        *result = make_handle<std::any>(tx_generated::dynamic_struct(
            {type_name, "message", std::move(fields)}));
    }, tx::error_kind::io);
}

extern "C" int txrt_ws_receive_binary(const void* peer,
    std::int64_t timeout, const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        auto value = tx_generated::ws_receive_binary(resource_id(peer), timeout);
        tx_generated::struct_fields fields(2);
        fields[0] = {"open", value.open};
        fields[1] = {"data", bytes_from(value.text)};
        *result = make_handle<std::any>(tx_generated::dynamic_struct(
            {type_name, "binary_message", std::move(fields)}));
    }, tx::error_kind::io);
}

extern "C" int txrt_ws_receive_binary_stream(const void* peer,
    const void* destination, std::int64_t max_message_bytes,
    std::int64_t timeout, const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        auto value = tx_generated::ws_receive_binary_stream(resource_id(peer),
            std::any_cast<const tx_generated::binary_stream&>(
                *static_cast<const std::any*>(destination)),
            max_message_bytes, timeout);
        tx_generated::struct_fields fields(2);
        fields[0] = {"open", value.open};
        fields[1] = {"length", value.length};
        *result = make_handle<std::any>(tx_generated::dynamic_struct(
            {type_name, "stream_message", std::move(fields)}));
    }, tx::error_kind::io);
}

extern "C" int txrt_ws_reply_once(const void* peer, void* handler,
    std::int64_t timeout, bool* result) noexcept
{
    return invoke_checked([&]
    {
        auto* owned = make_handle<std::any>(*static_cast<const std::any*>(peer));
        *result = tx_callback_m0_bridge_reply_once_0(owned, handler, timeout);
    }, tx::error_kind::io);
}

extern "C" int txrt_ws_reply_once_binary(const void* peer, void* handler,
    std::int64_t timeout, bool* result) noexcept
{
    return invoke_checked([&]
    {
        auto* owned = make_handle<std::any>(*static_cast<const std::any*>(peer));
        *result = tx_callback_m0_bridge_reply_once_binary_0(owned, handler,
                                                              timeout);
    }, tx::error_kind::io);
}

extern "C" int txrt_ws_close_listener(const void* server) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::ws_close_listener(resource_id(server));
    }, tx::error_kind::io);
}

extern "C" int txrt_ws_close_connection(const void* peer) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::ws_close_connection(resource_id(peer));
    }, tx::error_kind::io);
}
