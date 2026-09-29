#include "backend/cpp/external_direct_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/stdlib.hpp"

#include <any>
#include <string>

using tx_generated::detail::invoke_checked;

extern "C" int txrt_format_format(const void* text, const void* args,
                                    const void* kwargs, void** result) noexcept
{
    return invoke_checked([&] {
        const auto& values = std::any_cast<const tx_generated::tx_array&>(
            *static_cast<const std::any*>(args));
        const auto& keywords = std::any_cast<const tx_generated::tx_dict&>(
            *static_cast<const std::any*>(kwargs));
        *result = tx_generated::detail::make_handle<std::string>(tx_generated::tx_fn_format(
            tx_generated::detail::text_value(text), values, keywords));
    });
}
