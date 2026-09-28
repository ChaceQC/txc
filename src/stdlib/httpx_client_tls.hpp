#pragma once

#include "stdlib/secret.hpp"
#include "stdlib/vector.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace tx_generated::network
{
class socket_handle;
}

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <windows.h>
#include <winhttp.h>

namespace tx_generated::httpx_client_tls
{

struct settings;
}

namespace tx_generated::tls
{
class secure_connection;
}

namespace tx_generated::httpx_client_tls
{

std::shared_ptr<const settings> create(const bytes_vector& anchors,
    bool include_system, const byte_value& package,
    const secret::handle& password);
bool has_custom_anchors(const settings& value) noexcept;
std::shared_ptr<tls::secure_connection> connect_custom(
    const settings& value, std::shared_ptr<network::socket_handle> socket,
    std::string_view hostname, const std::vector<std::string>& protocols,
    std::int64_t timeout_ms, bool allow_no_alpn = false);
void configure(HINTERNET request, const settings& value);
void verify(HINTERNET request, std::string_view hostname,
    const settings& value);

} // namespace tx_generated::httpx_client_tls
