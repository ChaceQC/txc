#pragma once

#include "stdlib/httpx_client_tls.hpp"
#include "stdlib/network_common.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace tx_generated::tls
{
class secure_connection;
}

namespace tx_generated::httpx_custom_tls
{

std::string resolve_system_proxy(HINTERNET session,
    std::wstring_view target_host, std::string_view target_url, bool secure);
std::string request_authority(const network::parsed_url& address);
void send_tls_data(const std::shared_ptr<tls::secure_connection>& connection,
    std::string_view data, std::int64_t timeout_ms);
std::string validate_status_line(const network::parsed_head& head);
bool response_header_has_token(const network::header_map& headers,
    std::string_view name, std::string_view token);
std::shared_ptr<tls::secure_connection> open_verified_connection(
    const httpx_client_tls::settings& tls, std::string_view proxy_url,
    const network::parsed_url& target,
    const std::vector<std::string>& protocols, std::int64_t timeout_ms,
    bool allow_no_alpn);

} // namespace tx_generated::httpx_custom_tls
