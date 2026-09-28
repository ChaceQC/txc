#pragma once

#include "stdlib/httpx.hpp"
#include "stdlib/secret.hpp"

#include <cstdint>
#include <string_view>

namespace tx_generated::http3
{

bool is_http3_id(std::int64_t id) noexcept;
std::int64_t listen(std::string_view host, std::int64_t port,
                    const byte_value& package,
                    const secret::handle& password);
std::int64_t accept(std::int64_t listener, const binary_stream& destination,
                    std::size_t max_request_bytes, bool binary,
                    std::int64_t timeout_ms);
http_request_data request(std::int64_t id);
void respond(std::int64_t id, const http_response_data& value,
             const binary_stream& source, std::int64_t body_length,
             bool binary);
void close_listener(std::int64_t id) noexcept;
void close_connection(std::int64_t id) noexcept;

} // namespace tx_generated::http3
