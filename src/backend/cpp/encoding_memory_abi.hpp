#pragma once

#include "common/text_encoding.hpp"

extern "C"
{

int txrt_encoding_encode_known(const void* source, std::int64_t selected, void** result) noexcept;
int txrt_encoding_encode_literal(const char* source, std::uint64_t length,
    std::int64_t selected, void** result) noexcept;
int txrt_encoding_decode_known(const void* source, std::int64_t selected, void** result) noexcept;
int txrt_encoding_encode_known_length(const void* source, std::int64_t selected, std::int64_t* result) noexcept;
int txrt_encoding_encode_literal_length(const char* source, std::uint64_t length,
    std::int64_t selected, std::int64_t* result) noexcept;
int txrt_encoding_decode_known_length(const void* source, std::int64_t selected, std::int64_t* result) noexcept;

}
