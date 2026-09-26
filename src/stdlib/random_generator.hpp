#pragma once

#include <cstddef>
#include <cstdint>

namespace tx_generated::random_instance
{

std::uint64_t next_word(void* source);
std::uint64_t next_index(void* source, std::uint64_t bound);
[[noreturn]] void fail(const char* code, const char* message);

} // namespace tx_generated::random_instance
