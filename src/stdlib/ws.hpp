#pragma once

#include "stdlib/network_common.hpp"
#include "stdlib/file_stream.hpp"
#include "stdlib/tls_stream.hpp"
#include "stdlib/httpx.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace tx_generated
{

struct ws_message_data
{
    bool open = false;
    std::string text;
    std::int64_t close_code = 1005;
    std::string close_reason;
};

struct ws_stream_message_data
{
    bool open = false;
    std::int64_t length = 0;
    std::int64_t close_code = 1005;
    std::string close_reason;
};

struct ws_close_status_data
{
    bool received = false;
    std::int64_t code = 1005;
    std::string reason;
};

struct ws_connection_state
{
    network::http_handle session;
    network::http_handle connection;
    network::http_handle websocket;
    std::unique_ptr<network::tcp_stream> stream;
    std::shared_ptr<void> listener_slot;
    ws_close_status_data close_status;
    bool open = true;
    bool server = false;
};

std::shared_ptr<ws_connection_state> ws_client_connect(std::string_view url,
                                                        std::int64_t timeout_ms);
void ws_client_send(ws_connection_state& state, std::string_view data,
                    bool binary);
void ws_client_send_stream(ws_connection_state& state,
                           const binary_stream& source, std::int64_t length);
ws_message_data ws_client_receive(std::shared_ptr<ws_connection_state> state,
                                  std::int64_t timeout_ms, bool binary);
ws_stream_message_data ws_client_receive_stream(
    std::shared_ptr<ws_connection_state> state,
    const binary_stream& destination, std::int64_t max_message_bytes,
    std::int64_t timeout_ms);
void ws_client_close(ws_connection_state& state,
                     std::uint16_t code = 1000,
                     std::string_view reason = {}) noexcept;

std::shared_ptr<ws_connection_state> ws_server_accept(network::socket_handle socket,
                                                       std::int64_t timeout_ms);
std::shared_ptr<ws_connection_state> ws_server_accept(
    std::unique_ptr<network::tcp_stream> stream, std::int64_t timeout_ms);
std::shared_ptr<ws_connection_state> ws_server_upgrade(
    httpx_upgraded_connection accepted);
void ws_server_send(ws_connection_state& state, std::string_view data,
                    bool binary);
void ws_server_send_stream(ws_connection_state& state,
                           const binary_stream& source, std::int64_t length);
ws_message_data ws_server_receive(ws_connection_state& state,
                                  std::int64_t timeout_ms, bool binary);
ws_stream_message_data ws_server_receive_stream(
    ws_connection_state& state, const binary_stream& destination,
    std::int64_t max_message_bytes, std::int64_t timeout_ms);
void ws_server_close(ws_connection_state& state,
                     std::uint16_t code = 1000,
                     std::string_view reason = {}) noexcept;

std::int64_t ws_connect(std::string_view url, std::int64_t timeout_ms);
std::int64_t ws_listen(std::string_view host, std::int64_t port);
std::int64_t ws_listen_tls(std::string_view host, std::int64_t port,
                           tls::server_options options);
std::int64_t ws_upgrade_http(std::int64_t connection);
std::int64_t ws_accept(std::int64_t listener, std::int64_t timeout_ms);
void ws_send_text(std::int64_t id, std::string_view text);
void ws_send_binary(std::int64_t id, std::string_view data);
void ws_send_binary_stream(std::int64_t id, const binary_stream& source,
                           std::int64_t length);
ws_message_data ws_receive(std::int64_t id, std::int64_t timeout_ms);
ws_message_data ws_receive_binary(std::int64_t id, std::int64_t timeout_ms);
ws_stream_message_data ws_receive_binary_stream(
    std::int64_t id, const binary_stream& destination,
    std::int64_t max_message_bytes, std::int64_t timeout_ms);
bool ws_is_open(std::int64_t id);
ws_close_status_data ws_close_status(std::int64_t id);
void ws_send_ping(std::int64_t id, std::string_view data);
void ws_send_pong(std::int64_t id, std::string_view data);
void ws_close_with_reason(std::int64_t id, std::int64_t code,
                          std::string_view reason);
void ws_close_listener(std::int64_t id) noexcept;
void ws_close_connection(std::int64_t id) noexcept;

} // namespace tx_generated
