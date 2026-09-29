#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/value_format.hpp"
#include "stdlib/dns.hpp"
#include "stdlib/network_common.hpp"
#include "stdlib/vector.hpp"

#include <any>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{

void* make_addresses(std::vector<tx_generated::dns::address> addresses,
                     const char* vector_type, const char* address_type)
{
    tx_generated::object_vector result(vector_type);
    result.data().values.reserve(addresses.size());
    for (auto& address : addresses)
    {
        tx_generated::struct_fields fields(3);
        fields[0] = {"ip", std::move(address.ip)};
        fields[1] = {"family", std::move(address.family)};
        fields[2] = {"ttl_seconds", address.ttl_seconds};
        result.data().values.emplace_back(tx_generated::dynamic_struct(
            tx_generated::dynamic_struct_data{address_type, "address",
                std::move(fields)}));
    }
    result.data().refresh();
    return tx_generated::detail::make_handle<std::any>(std::move(result));
}

std::shared_ptr<tx_generated::cancellation_state> token_state(const void* value)
{
    const auto& token = std::any_cast<const tx_generated::cancel_token&>(
        *static_cast<const std::any*>(value));
    if (!token.state)
    {
        tx_generated::network::fail("invalid_argument", "DNS 取消令牌无效");
    }
    return token.state;
}

} // namespace

extern "C" int txrt_dns_resolve(const void* host, std::int64_t timeout_ms,
    const char* vector_type, const char* address_type, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = make_addresses(tx_generated::dns::resolve(
            tx_generated::detail::text_value(host), timeout_ms, {}),
            vector_type, address_type);
    }, tx::error_kind::io);
}

extern "C" int txrt_dns_resolve_with_cancel(const void* host,
    std::int64_t timeout_ms, const void* token, const char* vector_type,
    const char* address_type, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = make_addresses(tx_generated::dns::resolve(
            tx_generated::detail::text_value(host), timeout_ms,
            token_state(token)), vector_type, address_type);
    }, tx::error_kind::io);
}
