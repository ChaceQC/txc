#pragma once

#include "stdlib/httpx.hpp"
#include "stdlib/secret.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace tx_generated::httpx_session
{

struct response_chunk
{
    std::string data;
    bool eof = false;
};

std::int64_t open(std::string_view proxy_url, std::int64_t max_connections,
                   bool decompress);
std::int64_t open_secure(std::string_view proxy_url,
    std::int64_t max_connections, bool decompress,
    const bytes_vector& anchors, bool include_system,
    const byte_value& package, const secret::handle& password,
    bool allow_http2);
void close(std::int64_t session) noexcept;
std::int64_t begin(std::int64_t session, std::string_view method,
                   std::string_view url, const network::header_map& headers,
                   std::int64_t body_length, std::int64_t max_response_bytes,
                   std::int64_t timeout_ms, bool require_http2);
std::int64_t write(std::int64_t request, std::string_view data);
http_response_data finish(std::int64_t request);
response_chunk read(std::int64_t request, std::int64_t max_bytes);
void close_request(std::int64_t request) noexcept;

} // namespace tx_generated::httpx_session
