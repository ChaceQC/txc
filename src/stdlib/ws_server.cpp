#include "stdlib/ws.hpp"

#include "stdlib/error.hpp"
#include "stdlib/ws_frames.hpp"

#include <algorithm>
#include <cctype>
#include <utility>

namespace tx_generated
{
namespace
{

constexpr std::string_view websocket_guid =
    "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

bool contains_token(std::string_view text, std::string_view wanted)
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
        if (network::lower_ascii(token) == wanted)
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

bool valid_key(std::string_view key)
{
    if (key.size() != 24 || !key.ends_with("=="))
    {
        return false;
    }
    return std::all_of(key.begin(), key.begin() + 22, [](unsigned char value)
    {
        return (value >= 'A' && value <= 'Z') ||
            (value >= 'a' && value <= 'z') ||
            (value >= '0' && value <= '9') || value == '+' || value == '/';
    });
}

void handshake(network::tcp_stream& stream, network::parsed_head head)
{
    if (!head.cookies.empty())
    {
        network::fail("protocol_error", "WebSocket 握手请求不能包含 Set-Cookie");
    }
    const auto first = head.first_line.find(' ');
    const auto second = head.first_line.find(' ', first + 1);
    if (first == std::string::npos || second == std::string::npos ||
        head.first_line.substr(0, first) != "GET" ||
        head.first_line.substr(second + 1) != "HTTP/1.1" ||
        second == first + 1 || head.first_line[first + 1] != '/')
    {
        network::fail("protocol_error", "WebSocket 握手请求行无效");
    }
    const auto& headers = head.headers;
    const auto upgrade = headers.find("upgrade");
    const auto connection = headers.find("connection");
    const auto key = headers.find("sec-websocket-key");
    const auto version = headers.find("sec-websocket-version");
    if (!headers.contains("host") || upgrade == headers.end() ||
        connection == headers.end() || key == headers.end() ||
        version == headers.end() ||
        !contains_token(upgrade->second, "websocket") ||
        !contains_token(connection->second, "upgrade") ||
        version->second != "13" || !valid_key(key->second))
    {
        network::fail("protocol_error", "WebSocket 握手头无效");
    }
    if (headers.contains("content-length") &&
        network::content_length(headers, 0) != 0)
    {
        network::fail("protocol_error", "WebSocket Upgrade 不能携带请求正文");
    }
    const auto accept = network::base64(network::sha1(key->second +
        std::string(websocket_guid)));
    stream.send_all("HTTP/1.1 101 Switching Protocols\r\n"
                    "Upgrade: websocket\r\nConnection: Upgrade\r\n"
                    "Sec-WebSocket-Accept: " + accept + "\r\n\r\n");
}

void protocol_close(network::tcp_stream& stream, std::uint16_t status) noexcept
{
    try
    {
        const std::string code{static_cast<char>(status >> 8),
                               static_cast<char>(status & 0xff)};
        network::send_ws_frame(stream, 0x8, code);
    }
    catch (...)
    {
    }
    stream.close();
}

ws_close_status_data validate_close_payload(std::string_view payload)
{
    if (payload.size() == 1)
    {
        network::fail("protocol_error", "WebSocket close 帧长度无效");
    }
    if (payload.size() >= 2)
    {
        const auto code = (static_cast<unsigned char>(payload[0]) << 8) |
                           static_cast<unsigned char>(payload[1]);
        if (code < 1000 || code == 1004 || code == 1005 ||
            code == 1006 || code == 1015 ||
            (code >= 1016 && code < 3000) || code >= 5000)
        {
            network::fail("protocol_error", "WebSocket close 状态码无效");
        }
        network::validate_utf8(payload.substr(2));
        return {true, code, std::string(payload.substr(2))};
    }
    return {true, 1005, {}};
}

ws_message_data receive_message(network::tcp_stream& stream, bool binary)
{
    std::string text;
    bool fragmented = false;
    while (true)
    {
        auto frame = network::read_ws_frame(stream);
        switch (frame.opcode)
        {
        case 0x8:
        {
            auto closed = validate_close_payload(frame.payload);
            network::send_ws_frame(stream, 0x8, frame.payload);
            stream.close();
            return {false, {}, closed.code, std::move(closed.reason)};
        }
        case 0x9:
            network::send_ws_frame(stream, 0xA, frame.payload);
            continue;
        case 0xA:
            continue;
        case 0x2:
        case 0x1:
            if (fragmented)
            {
                network::fail("protocol_error", "WebSocket 分片消息未结束");
            }
            if (frame.opcode != (binary ? 0x2 : 0x1))
            {
                network::fail("unsupported_frame", "收到与读取接口类型不符的 WebSocket 消息");
            }
            break;
        case 0x0:
            if (!fragmented)
            {
                network::fail("protocol_error", "WebSocket 延续帧没有前置文本帧");
            }
            break;
        default:
            network::fail("protocol_error", "WebSocket 帧操作码无效");
        }
        if (text.size() + frame.payload.size() > network::max_body_bytes)
        {
            network::fail("size_limit", "WebSocket 文本消息超过 8 MiB");
        }
        text += frame.payload;
        fragmented = !frame.final;
        if (frame.final)
        {
            if (!binary)
            {
                network::validate_utf8(text);
            }
            return {true, std::move(text), 1005, {}};
        }
    }
}

ws_stream_message_data receive_stream_message(
    network::tcp_stream& stream, const binary_stream& destination,
    std::int64_t max_message_bytes)
{
    std::int64_t total = 0;
    bool fragmented = false;
    constexpr std::size_t block_size = 16 * 1024;
    while (true)
    {
        const auto frame = network::read_ws_frame_header(stream,
            std::max<std::size_t>(125, static_cast<std::size_t>(max_message_bytes)));
        if (frame.opcode == 0x8 || frame.opcode == 0x9 || frame.opcode == 0xA)
        {
            const auto payload = network::read_ws_payload(stream, frame, 0,
                                                           frame.length);
            if (frame.opcode == 0x8)
            {
                auto closed = validate_close_payload(payload);
                network::send_ws_frame(stream, 0x8, payload);
                stream.close();
                return {false, 0, closed.code, std::move(closed.reason)};
            }
            if (frame.opcode == 0x9)
            {
                network::send_ws_frame(stream, 0xA, payload);
            }
            continue;
        }
        if ((frame.opcode == 0x2 && fragmented) ||
            (frame.opcode == 0x0 && !fragmented))
        {
            network::fail("protocol_error", "WebSocket 二进制分片顺序无效");
        }
        if (frame.opcode != 0x2 && frame.opcode != 0x0)
        {
            network::fail(frame.opcode == 0x1 ? "unsupported_frame" :
                          "protocol_error", "收到非二进制 WebSocket 消息");
        }
        if (frame.length > static_cast<std::uint64_t>(max_message_bytes - total))
        {
            network::fail("size_limit", "WebSocket 消息超过接收上限");
        }
        for (std::size_t offset = 0; offset < frame.length;)
        {
            const auto amount = std::min(block_size, frame.length - offset);
            destination->file.write(network::read_ws_payload(stream, frame,
                                                               offset, amount));
            offset += amount;
        }
        total += static_cast<std::int64_t>(frame.length);
        fragmented = !frame.final;
        if (frame.final)
        {
            return {true, total, 1005, {}};
        }
    }
}

} // namespace

std::shared_ptr<ws_connection_state> ws_server_accept(network::socket_handle socket,
                                                       std::int64_t timeout_ms)
{
    return ws_server_accept(std::make_unique<network::tcp_stream>(
        std::move(socket)), timeout_ms);
}

std::shared_ptr<ws_connection_state> ws_server_accept(
    std::unique_ptr<network::tcp_stream> stream, std::int64_t timeout_ms)
{
    auto state = std::make_shared<ws_connection_state>();
    state->stream = std::move(stream);
    state->stream->set_receive_timeout(timeout_ms);
    try
    {
        handshake(*state->stream,
            network::parse_head(state->stream->read_head()));
    }
    catch (const runtime_failure& error)
    {
        if (error.error().code == "protocol_error" ||
            error.error().code == "invalid_header")
        {
            try
            {
                state->stream->send_all("HTTP/1.1 400 Bad Request\r\n"
                    "Content-Length: 0\r\nConnection: close\r\n\r\n");
            }
            catch (...)
            {
            }
        }
        throw;
    }
    state->server = true;
    return state;
}

std::shared_ptr<ws_connection_state> ws_server_upgrade(
    httpx_upgraded_connection accepted)
{
    auto state = std::make_shared<ws_connection_state>();
    state->stream = std::move(accepted.stream);
    state->listener_slot = std::move(accepted.listener_slot);
    state->server = true;
    if (accepted.request.body_length != 0)
    {
        network::fail("protocol_error", "WebSocket Upgrade 不能携带请求正文");
    }
    network::parsed_head head;
    head.first_line = accepted.request.method + " " +
        accepted.request.target + " HTTP/1.1";
    head.headers = std::move(accepted.request.headers);
    handshake(*state->stream, std::move(head));
    return state;
}

void ws_server_send(ws_connection_state& state, std::string_view data,
                    bool binary)
{
    network::send_ws_frame(*state.stream, binary ? 0x2 : 0x1, data);
}

void ws_server_send_stream(ws_connection_state& state,
                           const binary_stream& source, std::int64_t length)
{
    if (length == 0)
    {
        network::send_ws_frame(*state.stream, 0x2, {});
        return;
    }
    constexpr std::size_t block_size = 16 * 1024;
    char buffer[block_size];
    auto remaining = static_cast<std::uint64_t>(length);
    bool first = true;
    while (remaining > 0)
    {
        const auto count = source->file.read_into(buffer,
            static_cast<std::size_t>(std::min<std::uint64_t>(remaining,
                                                            block_size)));
        if (count == 0)
        {
            network::fail("operation_failed", "WebSocket 消息源流提前结束");
        }
        remaining -= count;
        network::send_ws_frame(*state.stream, first ? 0x2 : 0x0,
                               {buffer, count},
                               remaining == 0);
        first = false;
    }
}

ws_message_data ws_server_receive(ws_connection_state& state,
                                  std::int64_t timeout_ms, bool binary)
{
    state.stream->set_receive_timeout(timeout_ms);
    try
    {
        auto value = receive_message(*state.stream, binary);
        if (!value.open)
        {
            state.close_status = {true, value.close_code, value.close_reason};
        }
        return value;
    }
    catch (const runtime_failure& error)
    {
        const auto& code = error.error().code;
        if (code == "protocol_error" || code == "unsupported_frame" ||
            code == "size_limit" || code == "invalid_utf8")
        {
            const std::uint16_t status = code == "unsupported_frame" ? 1003 :
                code == "size_limit" ? 1009 : code == "invalid_utf8" ? 1007 : 1002;
            protocol_close(*state.stream, status);
            state.open = false;
        }
        throw;
    }
}

ws_stream_message_data ws_server_receive_stream(
    ws_connection_state& state, const binary_stream& destination,
    std::int64_t max_message_bytes, std::int64_t timeout_ms)
{
    state.stream->set_receive_timeout(timeout_ms);
    try
    {
        auto value = receive_stream_message(*state.stream, destination,
                                            max_message_bytes);
        if (!value.open)
        {
            state.close_status = {true, value.close_code, value.close_reason};
        }
        return value;
    }
    catch (const runtime_failure& error)
    {
        const auto& code = error.error().code;
        if (code == "protocol_error" || code == "unsupported_frame" ||
            code == "size_limit" || code == "invalid_utf8")
        {
            const std::uint16_t status = code == "unsupported_frame" ? 1003 :
                code == "size_limit" ? 1009 : code == "invalid_utf8" ? 1007 : 1002;
            protocol_close(*state.stream, status);
            state.open = false;
        }
        throw;
    }
}

void ws_server_close(ws_connection_state& state, std::uint16_t code,
                     std::string_view reason) noexcept
{
    if (state.open && state.stream && state.stream->valid())
    {
        try
        {
            std::string status{static_cast<char>(code >> 8),
                               static_cast<char>(code & 0xff)};
            status += reason;
            network::send_ws_frame(*state.stream, 0x8, status);
            state.stream->set_receive_timeout(1000);
            while (network::read_ws_frame(*state.stream).opcode != 0x8)
            {
                // 关闭期间只等待对端的 close 帧。
            }
        }
        catch (...)
        {
        }
    }
    if (state.stream)
    {
        state.stream->close();
    }
    state.open = false;
}

} // namespace tx_generated
