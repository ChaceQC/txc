#pragma once

#include "stdlib/httpx.hpp"

#include <cstdint>
#include <string_view>

namespace tx_generated
{

http_response_data httpx_send_negotiated(std::string_view method,
    std::string_view url, const network::header_map& headers,
    std::string_view body, std::int64_t timeout_ms,
    std::string_view policy, bool binary);

} // namespace tx_generated
