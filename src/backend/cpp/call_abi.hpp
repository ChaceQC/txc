#pragma once

#include <cstddef>

extern "C"
{

int txrt_keyword_set(void* keywords, const char* name,
                       const void* value) noexcept;
int txrt_keyword_merge(void* keywords, const void* source) noexcept;
int txrt_call_bind(const void* positional, const void* keywords,
                     const char* const* names, std::size_t fixed_count,
                     int accepts_args, int accepts_kwargs,
                     void** result) noexcept;

}
