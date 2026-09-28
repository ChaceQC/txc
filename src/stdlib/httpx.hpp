#pragma once

#include "stdlib/network_common.hpp"
#include "stdlib/file_stream.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace tx_generated
{

struct http_response_data
{
    std::int64_t status = 0;
    std::string protocol;
    network::header_map headers;
    std::vector<std::string> cookies;
    std::string body;
    std::int64_t body_length = 0;
};

struct http_request_data
{
    std::string method;
    std::string target;
    network::header_map headers;
    std::string body;
    std::int64_t body_length = 0;
};

struct httpx_upgraded_connection
{
    std::unique_ptr<network::tcp_stream> stream;
    std::shared_ptr<void> listener_slot;
    http_request_data request;
};

std::int64_t httpx_client_send(std::string_view method, std::string_view url,
                               const network::header_map& headers,
                               std::string_view body, std::int64_t timeout_ms,
                               bool binary = false, bool http2 = false);
http_response_data httpx_client_stream(std::string_view method,
                                       std::string_view url,
                                       const network::header_map& headers,
                                       const binary_stream& source,
                                       std::int64_t source_length,
                                       const binary_stream& destination,
                                       std::int64_t max_response_bytes,
                                       std::int64_t timeout_ms,
                                       bool http2 = false);
http_response_data httpx_response(std::int64_t id);
void httpx_release_response(std::int64_t id) noexcept;
std::int64_t httpx_listen(std::string_view host, std::int64_t port);
std::int64_t httpx_listen_with_limit(std::string_view host, std::int64_t port,
                                     std::int64_t max_connections);
std::int64_t httpx_accept(std::int64_t listener, std::int64_t timeout_ms,
                          bool binary = false);
std::int64_t httpx_accept_stream(std::int64_t listener,
                                 const binary_stream& destination,
                                 std::int64_t max_request_bytes,
                                 std::int64_t timeout_ms);
http_request_data httpx_request(std::int64_t id);
httpx_upgraded_connection httpx_take_upgrade_connection(std::int64_t id);
void httpx_respond(std::int64_t id, const http_response_data& value,
                   bool binary = false);
void httpx_respond_stream(std::int64_t id, const http_response_data& value,
                          const binary_stream& source,
                          std::int64_t body_length);
void httpx_close_listener(std::int64_t id) noexcept;
void httpx_close_connection(std::int64_t id) noexcept;

} // namespace tx_generated
