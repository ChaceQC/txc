#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/profile.hpp"

#include <filesystem>
#include <fstream>

namespace
{
bool automatic_profile = false;
}

extern "C" int txrt_profile_start(std::int64_t interval, std::int64_t maximum) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        tx_generated::profile_start(interval, maximum);
    });
}

extern "C" int txrt_profile_snapshot(void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::detail::make_handle<std::string>(
            tx_generated::profile_snapshot(false));
    });
}

extern "C" int txrt_profile_stop(void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::detail::make_handle<std::string>(
            tx_generated::profile_snapshot(true));
    });
}

extern "C" int txrt_profile_begin_span(const void* name, std::int64_t* result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::profile_begin_span(tx_generated::detail::text_value(name));
    });
}

extern "C" int txrt_profile_end_span(std::int64_t id, std::int64_t* result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::profile_end_span(id);
    });
}

extern "C" const char* txrt_profile_abi_enter(void* value, const char* name) noexcept
{
    auto& context = *static_cast<tx_generated::detail::runtime_context*>(value);
    const auto previous = context.profile_abi;
    context.profile_abi = name;
    context.profile_abi_frame = context.active_frame;
    return previous;
}

extern "C" int txrt_profile_auto_start(std::int64_t interval) noexcept
{
    const auto status = txrt_profile_start(interval, 100000);
    automatic_profile = status == 0;
    return status;
}

extern "C" int txrt_profile_auto_finish() noexcept
{
    return tx_generated::detail::invoke_checked([]
    {
        if (!automatic_profile)
        {
            return;
        }
        automatic_profile = false;
        const auto report = tx_generated::profile_snapshot(true);
        std::ofstream output(std::filesystem::path("profile.json"), std::ios::binary);
        output << report << '\n';
        output.close();
        if (!output)
        {
            throw tx_generated::runtime_failure({tx::error_kind::io,
                "profile_io_error", "性能报告写入失败"});
        }
    });
}
