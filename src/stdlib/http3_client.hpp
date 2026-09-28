#pragma once

#include "stdlib/httpx.hpp"
#include "stdlib/cancellation.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

namespace tx_generated::http3
{

http_response_data client_send(std::string_view method,
                               std::string_view url,
                               const network::header_map& headers,
                               std::string_view body,
                               std::int64_t timeout_ms,
                               bool binary,
                               const std::vector<byte_value>& trust_anchors = {},
                               const cancel_token* token = nullptr);

} // namespace tx_generated::http3
