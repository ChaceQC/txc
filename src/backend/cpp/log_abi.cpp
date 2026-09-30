#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/checked_callback.hpp"
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
    return tx_generated::detail::text_value(value);
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

extern "C" int txrt_log_set_request_context(const void* request) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::detail::current_runtime_context().log_request_id = text(request);
    });
}

extern "C" int txrt_log_clear_context() noexcept
{
    return invoke_checked([]
    {
        auto& context = tx_generated::detail::current_runtime_context();
        context.log_request_id.clear();
        context.log_task_id.clear();
        context.log_thread_id.clear();
    });
}

extern "C" int txrt_log_set_file(const void* path, std::int64_t maximum,
    std::int64_t backups) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::tx_log_set_file(text(path), maximum, backups);
    }, tx::error_kind::io);
}

extern "C" int txrt_log_set_stderr() noexcept
{
    return invoke_checked([]
    {
        tx_generated::tx_log_set_stderr();
    });
}

extern "C" int txrt_log_flush() noexcept
{
    return invoke_checked([]
    {
        tx_generated::tx_log_flush();
    });
}

extern "C" int txrt_log_enabled(const void* level, bool* result) noexcept
{
    return tx_generated::detail::invoke_leaf([&]
    {
        *result = tx_generated::tx_log_enabled(text(level));
    });
}

extern "C" int txrt_log_enabled_literal(const char* level, std::uint64_t length,
    bool* result) noexcept
{
    return tx_generated::detail::invoke_leaf([&]
    {
        *result = tx_generated::tx_log_enabled(std::string(level, length));
    });
}

extern "C" int txrt_log_set_level(const void* level) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::tx_log_set_level(text(level));
    });
}

extern "C" int txrt_log_set_secret_keys(const void* values) noexcept
{
    return invoke_checked([&]
    {
        const auto& keys = std::any_cast<const tx_generated::string_vector&>(
            *static_cast<const std::any*>(values));
        std::vector<std::string> copied;
        for (const auto& key : keys.data().values)
        {
            copied.push_back(key.get());
        }
        tx_generated::tx_log_set_secret_keys(std::move(copied));
    });
}

extern "C" int txrt_log_event_lazy(const void* level, const void* message,
    const void* fields) noexcept
{
    return invoke_checked([&]
    {
        if (!tx_generated::tx_log_enabled(text(level)))
        {
            return;
        }
        auto* value = static_cast<std::any*>(tx_generated::invoke_typed_callback<void*>(fields));
        const auto cleanup = [](std::any* pointer)
        {
            tx_generated::detail::destroy_handle(pointer);
        };
        std::unique_ptr<std::any, decltype(cleanup)> owned(value, cleanup);
        tx_generated::tx_log_event(text(level), text(message), dictionary(value), {});
    });
}
