#include "backend/cpp/format_arguments_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/format_internal.hpp"

#include <bit>

extern "C" void tx_format_argument_i64(std::string& output, std::uint64_t bits,
    const tx::format_spec& spec, char conversion)
{
    tx_generated::append_format_value(output, std::bit_cast<std::int64_t>(bits), spec, conversion);
}

extern "C" void tx_format_argument_f64(std::string& output, std::uint64_t bits,
    const tx::format_spec& spec, char conversion)
{
    tx_generated::append_format_value(output, std::bit_cast<double>(bits), spec, conversion);
}

extern "C" void tx_format_argument_bool(std::string& output, std::uint64_t bits,
    const tx::format_spec& spec, char conversion)
{
    tx_generated::append_format_value(output, bits != 0, spec, conversion);
}

extern "C" void tx_format_argument_str(std::string& output, std::uint64_t bits,
    const tx::format_spec& spec, char conversion)
{
    tx_generated::append_format_value(output,
        tx_generated::detail::text_value(reinterpret_cast<const void*>(bits)), spec, conversion);
}

extern "C" int txrt_format_arguments_context(void* context, const void* text,
    const tx_generated::format_argument* arguments, std::uint64_t positional,
    std::uint64_t total, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        const std::span<const tx_generated::format_argument> values(arguments, total);
        auto output = tx_generated::format_direct(tx_generated::detail::text_value(text),
            values.first(positional), values.subspan(positional));
        *result = tx_generated::detail::make_handle<std::string>(std::move(output));
    }, tx::error_kind::runtime, static_cast<tx_generated::detail::runtime_context*>(context));
}
