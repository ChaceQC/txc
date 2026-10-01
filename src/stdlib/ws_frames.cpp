#include "stdlib/ws_frames.hpp"
#include "stdlib/crypto_internal.hpp"

#include <algorithm>
#include <array>
#include <limits>

namespace tx_generated::network
{

void send_ws_frame(tcp_stream& stream, std::uint8_t opcode,
                   std::string_view payload, bool final)
{
    // RFC 6455 要求每个客户端帧使用独立的不可预测掩码。
    const bool masked = stream.websocket_client();
    const auto mask_flag = masked ? 0x80 : 0;
    std::string header;
    header.push_back(static_cast<char>((final ? 0x80 : 0) | opcode));
    if (payload.size() <= 125)
    {
        header.push_back(static_cast<char>(payload.size() | mask_flag));
    }
    else if (payload.size() <= 65535)
    {
        header.push_back(static_cast<char>(126 | mask_flag));
        header.push_back(static_cast<char>((payload.size() >> 8) & 0xff));
        header.push_back(static_cast<char>(payload.size() & 0xff));
    }
    else
    {
        header.push_back(static_cast<char>(127 | mask_flag));
        for (int shift = 56; shift >= 0; shift -= 8)
        {
            header.push_back(static_cast<char>((payload.size() >> shift) & 0xff));
        }
    }
    std::array<std::uint8_t, 4> mask{};
    if (masked)
    {
        crypto::fill_random(mask);
        header.append(reinterpret_cast<const char*>(mask.data()), mask.size());
    }
    // 短帧必须把头和正文一次交给 TCP，避免分开发送触发 Nagle/延迟确认。
    // 大帧仍按块掩码，避免复制整个消息。
    if (payload.size() <= 16 * 1024)
    {
        const auto offset = header.size();
        header.append(payload);
        if (masked)
        {
            for (std::size_t index = 0; index < payload.size(); ++index)
            {
                header[offset + index] ^= static_cast<char>(mask[index % 4]);
            }
        }
        stream.send_all(header);
        return;
    }
    stream.send_all(header);
    if (!masked)
    {
        stream.send_all(payload);
        return;
    }
    for (std::size_t offset = 0; offset < payload.size();)
    {
        std::string block(payload.substr(offset, 16 * 1024));
        for (std::size_t index = 0; index < block.size(); ++index)
        {
            block[index] ^= static_cast<char>(mask[(offset + index) % 4]);
        }
        stream.send_all(block);
        offset += block.size();
    }
}

ws_frame_header read_ws_frame_header(tcp_stream& stream,
                                     std::size_t max_payload)
{
    const auto first = stream.read_exact(2);
    const auto flags = static_cast<std::uint8_t>(first[0]);
    const auto length_flag = static_cast<std::uint8_t>(first[1]);
    ws_frame_header result;
    result.final = (flags & 0x80) != 0;
    result.opcode = flags & 0x0f;
    const bool masked = (length_flag & 0x80) != 0;
    if ((flags & 0x70) != 0 || masked == stream.websocket_client())
    {
        fail("protocol_error", "WebSocket 帧掩码方向或保留位无效");
    }
    std::uint64_t length = length_flag & 0x7f;
    if (length == 126)
    {
        const auto bytes = stream.read_exact(2);
        length = (static_cast<unsigned char>(bytes[0]) << 8) |
                 static_cast<unsigned char>(bytes[1]);
        if (length < 126)
        {
            fail("protocol_error", "WebSocket 帧长度编码不是最短形式");
        }
    }
    else if (length == 127)
    {
        const auto bytes = stream.read_exact(8);
        length = 0;
        for (unsigned char value : bytes)
        {
            length = (length << 8) | value;
        }
        if (length < 65536 || (length >> 63) != 0)
        {
            fail("protocol_error", "WebSocket 帧长度编码无效");
        }
    }
    const bool control = (result.opcode & 0x08) != 0;
    if ((control && (!result.final || length > 125)) ||
        length > max_payload)
    {
        fail(length > max_payload ? "size_limit" : "protocol_error",
             "WebSocket 帧长度或控制帧格式无效");
    }
    if (masked)
    {
        const auto mask = stream.read_exact(4);
        std::copy_n(mask.begin(), 4, result.mask.begin());
    }
    result.length = static_cast<std::size_t>(length);
    return result;
}

std::string read_ws_payload(tcp_stream& stream, const ws_frame_header& header,
                            std::size_t offset, std::size_t length)
{
    auto result = stream.read_exact(length);
    for (std::size_t index = 0; index < result.size(); ++index)
    {
        result[index] ^= header.mask[(offset + index) % 4];
    }
    return result;
}

ws_frame read_ws_frame(tcp_stream& stream)
{
    const auto header = read_ws_frame_header(stream, max_body_bytes);
    return {header.opcode, header.final,
            read_ws_payload(stream, header, 0, header.length)};
}

} // namespace tx_generated::network
