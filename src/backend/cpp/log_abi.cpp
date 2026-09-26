#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/log.hpp"
#include "stdlib/vector.hpp"

#include <any>
#include <string>
#include <vector>

namespace
{

using tx_generated::detail::invoke_checked;

const std::string& text(const void* value)
{
    return *static_cast<const std::string*>(value);
}

const tx_generated::tx_dict& dictionary(const void* value)
{
    return std::any_cast<const tx_generated::tx_dict&>(
        *static_cast<const std::any*>(value));
}

} // namespace

extern "C" int txrt_log_event(const void* level, const void* message,
                               const void* fields) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::tx_log_event(text(level), text(message),
                                   dictionary(fields), {});
    });
}

extern "C" int txrt_log_event_redacted(const void* level,
    const void* message, const void* fields,
    const void* secret_keys) noexcept
{
    return invoke_checked([&]
    {
        std::vector<std::string> keys;
        const auto& values = std::any_cast<const tx_generated::string_vector&>(
            *static_cast<const std::any*>(secret_keys));
        keys.reserve(values.data().values.size());
        for (const auto& item : values.data().values)
        {
            keys.push_back(item.get());
        }
        tx_generated::tx_log_event(text(level), text(message),
                                   dictionary(fields), keys);
    });
}

extern "C" int txrt_log_set_context(const void* task,
                                      const void* thread) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::tx_log_set_context(text(task), text(thread));
    });
}
