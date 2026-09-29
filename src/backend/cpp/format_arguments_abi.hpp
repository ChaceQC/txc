#pragma once

#include "common/format_spec.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace tx_generated
{

struct format_argument
{
    const char* name;
    void (*append)(std::string&, std::uint64_t, const tx::format_spec&, char);
    std::uint64_t bits;
};

static_assert(sizeof(format_argument) == 24 && offsetof(format_argument, bits) == 16);

std::string format_direct(const std::string& text, std::span<const format_argument> positional,
    std::span<const format_argument> keywords);

} // namespace tx_generated

extern "C"
{

int txrt_format_arguments_context(void* context, const void* text,
    const tx_generated::format_argument* arguments, std::uint64_t positional,
    std::uint64_t total, void** result) noexcept;
void tx_format_argument_i64(std::string& output, std::uint64_t bits,
    const tx::format_spec& spec, char conversion);
void tx_format_argument_f64(std::string& output, std::uint64_t bits,
    const tx::format_spec& spec, char conversion);
void tx_format_argument_bool(std::string& output, std::uint64_t bits,
    const tx::format_spec& spec, char conversion);
void tx_format_argument_str(std::string& output, std::uint64_t bits,
    const tx::format_spec& spec, char conversion);

}
