#pragma once

#include "common/utf8.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace tx_generated::json_detail
{

using tx::utf8_width;

inline void append_utf8(std::string& output, std::uint32_t codepoint)
{
    if (codepoint <= 0x7f)
    {
        output.push_back(static_cast<char>(codepoint));
    }
    else if (codepoint <= 0x7ff)
    {
        output.push_back(static_cast<char>(0xc0 | (codepoint >> 6)));
        output.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
    }
    else if (codepoint <= 0xffff)
    {
        output.push_back(static_cast<char>(0xe0 | (codepoint >> 12)));
        output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
        output.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
    }
    else
    {
        output.push_back(static_cast<char>(0xf0 | (codepoint >> 18)));
        output.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3f)));
        output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
        output.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
    }
}

} // namespace tx_generated::json_detail
