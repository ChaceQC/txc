#pragma once

#include "common/text_encoding.hpp"

extern "C"
{

int txrt_encoding_encode_known(const void* source, std::int64_t selected, void** result) noexcept;
int txrt_encoding_decode_known(const void* source, std::int64_t selected, void** result) noexcept;

}
