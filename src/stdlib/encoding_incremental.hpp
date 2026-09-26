#pragma once

#include "stdlib/vector.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace tx_generated
{

struct codec_state;

struct encoding_decoder
{
    std::shared_ptr<codec_state> state;
};

struct encoding_encoder
{
    std::shared_ptr<codec_state> state;
};

encoding_decoder new_decoder(std::string_view encoding, std::string_view policy);
encoding_encoder new_encoder(std::string_view encoding, std::string_view policy);
std::string decode_chunk(const encoding_decoder& source,
                         const byte_value& data, bool eof);
byte_value encode_chunk(const encoding_encoder& target,
                        std::string_view text, bool eof);
std::int64_t decoder_replacements(const encoding_decoder& source);
std::int64_t encoder_replacements(const encoding_encoder& target);

} // namespace tx_generated
