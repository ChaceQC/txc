#include "stdlib/ws.hpp"
#include "stdlib/crypto_internal.hpp"
#include "stdlib/dns.hpp"
#include "stdlib/encoding.hpp"
#include "stdlib/error.hpp"
#include "stdlib/socket.hpp"
#include "stdlib/ws_frames.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <limits>

namespace tx_generated
{
namespace
{
std::int64_t remaining(std::chrono::steady_clock::time_point deadline)
{
    const auto amount = std::chrono::duration_cast<std::chrono::milliseconds>(
        deadline - std::chrono::steady_clock::now()).count();
    if (amount <= 0)
    {
        network::fail("timeout", "WebSocket 连接超时");
    }
    return amount;
}

std::shared_ptr<network::socket_handle> connect_socket(const network::parsed_url& url,
    std::chrono::steady_clock::time_point deadline)
{
    const auto host = detail::wide_to_utf8(url.host);
    const auto addresses = dns::resolve(host, std::min<std::int64_t>(60000, remaining(deadline)), {});
    std::exception_ptr failure;
    for (const auto& address : addresses)
    {
        try
        {
            auto peer = socket::connect_tcp(address.ip, url.port,
                std::min<std::int64_t>(60000, remaining(deadline)));
            return socket::take_tcp(peer.id);
        }
        catch (...)
        {
            failure = std::current_exception();
        }
    }
    if (failure)
    {
        std::rethrow_exception(failure);
    }
    network::fail("operation_failed", "连接 WebSocket 主机失败");
}

bool contains_token(std::string_view text, std::string_view expected)
{
    while (!text.empty())
    {
        const auto comma = text.find(',');
        auto token = text.substr(0, comma);
        while (!token.empty() && (token.front() == ' ' || token.front() == '\t'))
        {
            token.remove_prefix(1);
        }
        while (!token.empty() && (token.back() == ' ' || token.back() == '\t'))
        {
            token.remove_suffix(1);
        }
        if (network::lower_ascii(token) == expected)
        {
            return true;
        }
        if (comma == std::string_view::npos)
        {
            break;
        }
        text.remove_prefix(comma + 1);
    }
    return false;
}

void handshake(network::tcp_stream& stream, const network::parsed_url& url)
{
    std::array<std::uint8_t, 16> random{};
    crypto::fill_random(random);
    const auto key = network::base64({reinterpret_cast<const char*>(random.data()), random.size()});
    auto host = detail::wide_to_utf8(url.host);
    if (host.find(':') != std::string::npos)
    {
        host = "[" + host + "]";
    }
    host += ":" + std::to_string(url.port);
    stream.send_all("GET " + detail::wide_to_utf8(url.target) + " HTTP/1.1\r\nHost: " +
        host + "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Version: 13\r\n"
        "Sec-WebSocket-Key: " + key + "\r\n\r\n");
    const auto response = network::parse_head(stream.read_head());
    const auto& headers = response.headers;
    const auto upgrade = headers.find("upgrade");
    const auto connection = headers.find("connection");
    const auto accept = headers.find("sec-websocket-accept");
    const auto expected = network::base64(network::sha1(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"));
    if (!response.first_line.starts_with("HTTP/1.1 101 ") ||
        upgrade == headers.end() || connection == headers.end() || accept == headers.end() ||
        !contains_token(upgrade->second, "websocket") ||
        !contains_token(connection->second, "upgrade") || accept->second != expected ||
        headers.contains("sec-websocket-extensions") || headers.contains("sec-websocket-protocol"))
    {
        network::fail("protocol_error", "WebSocket 服务端握手响应无效");
    }
}
}

std::shared_ptr<ws_connection_state> ws_client_connect(std::string_view url,
    std::int64_t timeout_ms)
{
    if (timeout_ms < 1 || timeout_ms > std::numeric_limits<int>::max())
    {
        network::fail("invalid_argument", "WebSocket 连接超时无效");
    }
    const auto address = network::parse_url(url, true);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    auto native = connect_socket(address, deadline);
    auto state = std::make_shared<ws_connection_state>();
    if (address.secure)
    {
        tls::client_options options;
        options.hostname = detail::wide_to_utf8(address.host);
        options.allow_no_alpn = true;
        auto secure = std::make_shared<tls::secure_connection>(native, std::move(options),
            std::vector<std::string>{"http/1.1"}, remaining(deadline));
        state->stream = std::make_unique<network::tcp_stream>(std::move(secure));
    }
    else
    {
        unsigned long blocking = 0;
        if (ioctlsocket(native->get(), FIONBIO, &blocking) != 0)
        {
            network::socket_failure("设置 WebSocket 阻塞模式");
        }
        const DWORD timeout = static_cast<DWORD>(remaining(deadline));
        network::set_socket_option(native->get(), SOL_SOCKET, SO_SNDTIMEO,
            reinterpret_cast<const char*>(&timeout), sizeof(timeout));
        state->stream = std::make_unique<network::tcp_stream>(std::move(*native));
    }
    state->stream->set_websocket_client(true);
    state->stream->set_receive_timeout(timeout_ms);
    state->stream->set_receive_deadline(remaining(deadline));
    handshake(*state->stream, address);
    state->stream->clear_receive_deadline();
    return state;
}

void ws_client_send(ws_connection_state& state, std::string_view data, bool binary)
{
    ws_server_send(state, data, binary);
}

void ws_client_send_stream(ws_connection_state& state,
    const binary_stream& source, std::int64_t length)
{
    ws_server_send_stream(state, source, length);
}

ws_message_data ws_client_receive(std::shared_ptr<ws_connection_state> state,
    std::int64_t timeout_ms, bool binary)
{
    return ws_server_receive(*state, timeout_ms, binary);
}

ws_stream_message_data ws_client_receive_stream(std::shared_ptr<ws_connection_state> state,
    const binary_stream& destination, std::int64_t max_message_bytes, std::int64_t timeout_ms)
{
    return ws_server_receive_stream(*state, destination, max_message_bytes, timeout_ms);
}

void ws_client_close(ws_connection_state& state, std::uint16_t code,
    std::string_view reason) noexcept
{
    ws_server_close(state, code, reason);
}
}
