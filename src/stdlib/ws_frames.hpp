#pragma once

#include "stdlib/network_common.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace tx_generated::network
{

struct ws_frame
{
    std::uint8_t opcode = 0;
    bool final = false;
    std::string payload;
};

struct ws_frame_header
{
    std::uint8_t opcode = 0;
    bool final = false;
    std::size_t length = 0;
    std::array<char, 4> mask{};
};

void send_ws_frame(tcp_stream& stream, std::uint8_t opcode,
                   std::string_view payload, bool final = true);
ws_frame_header read_ws_frame_header(tcp_stream& stream,
                                     std::size_t max_payload);
std::string read_ws_payload(tcp_stream& stream, const ws_frame_header& header,
                            std::size_t offset, std::size_t length);
ws_frame read_ws_frame(tcp_stream& stream);

} // namespace tx_generated::network
