#pragma once

#include "stdlib/cbor.hpp"
#include "stdlib/array.hpp"
#include "stdlib/dictionary.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_set>

namespace tx_generated
{

struct cbor_head
{
    std::uint8_t major = 0;
    std::uint8_t additional = 0;
    std::uint64_t argument = 0;
};

cbor_head cbor_read_head(format_input& input);
std::any cbor_parse_value(format_input& input, const cbor_limits& limits,
                          std::size_t depth);
void cbor_skip_value(format_input& input, const cbor_limits& limits, std::size_t depth);
void cbor_require_end(format_input& input);
bool cbor_valid_utf8(std::string_view value);
bool cbor_key_less(std::string_view left, std::string_view right);
std::string cbor_head_bytes(std::uint8_t major, std::uint64_t argument);
std::string cbor_float_bytes(double value);
double cbor_half_value(std::uint16_t bits);

class cbor_output
{
public:
    cbor_output(format_sink sink, const cbor_limits& limits);
    void append(std::string_view bytes);
    void begin_value();
    void end_value();
    void flush();
    [[nodiscard]] std::uint64_t written() const noexcept;

private:
    format_sink sink_;
    std::string buffer_;
    cbor_limits limits_;
    std::uint64_t written_ = 0;
    std::uint64_t value_start_ = 0;
    bool within_value_ = false;
};

void cbor_emit_value(const std::any& value, cbor_output& output,
                     const cbor_limits& limits, std::size_t depth,
                     std::unordered_set<const void*>& active);
std::string cbor_encode_key(const std::any& key);

} // namespace tx_generated
