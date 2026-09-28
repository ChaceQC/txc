#include "backend/cpp/tls_abi_helpers.hpp"

#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/socket_abi_helpers.hpp"
#include "backend/cpp/value_format.hpp"
#include "stdlib/bytes.hpp"
#include "stdlib/tls_stream.hpp"

#include <any>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{

using tx_generated::dynamic_struct;
using tx_generated::struct_fields;

std::int64_t stream_id(const void* value)
{
    const auto& object = std::any_cast<const dynamic_struct&>(
        *static_cast<const std::any*>(value));
    if (object->fields.size() != 1)
    {
        tx_generated::network::fail("invalid_argument", "TLS 流句柄字段无效");
    }
    return std::any_cast<std::int64_t>(object->fields[0].value);
}

std::vector<std::string> protocols_at(const void* value)
{
    const auto& source = std::any_cast<const tx_generated::string_vector&>(
        *static_cast<const std::any*>(value));
    std::vector<std::string> result;
    result.reserve(source.data().values.size());
    for (const auto& protocol : source.data().values)
    {
        result.push_back(protocol.get());
    }
    return result;
}

void* stream_value(const char* type_name,
                   tx_generated::tls::stream_resource resource)
{
    struct_fields fields(1);
    fields[0] = {"id", resource.id};
    std::shared_ptr<const void> owner = std::move(resource.value);
    return tx_generated::detail::make_handle<std::any>(dynamic_struct(
        tx_generated::dynamic_struct_data{type_name, "secure_stream",
            std::move(fields), std::move(owner)}));
}

void* read_value(const char* type_name,
                 tx_generated::socket::read_result value)
{
    struct_fields fields(2);
    fields[0] = {"data", tx_generated::make_bytes(std::move(value.data))};
    fields[1] = {"eof", value.eof};
    return tx_generated::detail::make_handle<std::any>(dynamic_struct(
        tx_generated::dynamic_struct_data{type_name, "read_result",
            std::move(fields)}));
}

std::string_view bytes_at(const void* value)
{
    const auto& bytes = tx_generated::bytes_of(
        *static_cast<const std::any*>(value));
    return {bytes->empty() ? "" :
        reinterpret_cast<const char*>(bytes->data()), bytes->size()};
}

} // namespace

extern "C" int txrt_tls_connect(const void* peer, const void* config,
    const void* alpn, std::int64_t timeout_ms, const char* type_name,
    void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        auto options = tx_generated::tls_abi::checked_client(config);
        auto protocols = protocols_at(alpn);
        tx_generated::tls::validate_alpn(protocols, timeout_ms);
        auto native = tx_generated::socket::take_tcp(
            tx_generated::socket_abi::resource_id(peer));
        auto secured = std::make_shared<tx_generated::tls::secure_connection>(
            std::move(native), std::move(options),
            std::move(protocols), timeout_ms);
        *result = stream_value(type_name,
            tx_generated::tls::register_stream(std::move(secured)));
    }, tx::error_kind::security);
}

extern "C" int txrt_tls_accept(const void* peer, const void* config,
    const void* alpn, std::int64_t timeout_ms, const char* type_name,
    void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        auto options = tx_generated::tls_abi::checked_server(config);
        auto protocols = protocols_at(alpn);
        tx_generated::tls::validate_alpn(protocols, timeout_ms);
        auto native = tx_generated::socket::take_tcp(
            tx_generated::socket_abi::resource_id(peer));
        auto secured = std::make_shared<tx_generated::tls::secure_connection>(
            std::move(native), std::move(options),
            std::move(protocols), timeout_ms);
        *result = stream_value(type_name,
            tx_generated::tls::register_stream(std::move(secured)));
    }, tx::error_kind::security);
}

extern "C" int txrt_tls_read(const void* peer,
    std::int64_t max_bytes, std::int64_t timeout_ms,
    const char* type_name, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = read_value(type_name,
            tx_generated::tls::get_stream(stream_id(peer))->read(
                max_bytes, timeout_ms));
    }, tx::error_kind::io);
}

extern "C" int txrt_tls_write(const void* peer, const void* data,
    std::int64_t timeout_ms, std::int64_t* result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::tls::get_stream(stream_id(peer))->write(
            bytes_at(data), timeout_ms);
    }, tx::error_kind::io);
}

extern "C" int txrt_tls_negotiated_alpn(const void* peer,
    void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::detail::make_handle<std::string>(
            tx_generated::tls::get_stream(stream_id(peer))->negotiated_alpn());
    }, tx::error_kind::io);
}

extern "C" int txrt_tls_close(const void* peer,
    std::int64_t timeout_ms) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        tx_generated::tls::close_stream(stream_id(peer), timeout_ms);
    }, tx::error_kind::io);
}
