#include "backend/cpp/external_direct_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/env.hpp"
#include "stdlib/system.hpp"
#include "stdlib/vector.hpp"

#include <any>

namespace
{

const std::string& text_value(const void* value)
{
    return tx_generated::detail::text_value(value);
}

} // namespace

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

extern "C" int txrt_system_initialize() noexcept
{
    return invoke_checked([]
    {
        tx_generated::tx_prepare_system();
    });
}

extern "C" int txrt_system_args(void** result) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::string_vector snapshot;
        auto& values = snapshot.data().values;
        const auto& arguments = tx_generated::tx_fn_system_args();
        values.reserve(arguments.size());
        for (const auto& argument : arguments)
        {
            auto* text = make_handle<std::string>(argument);
            tx_generated::text_reference reference(text);
            tx_generated::detail::destroy_handle(text);
            values.push_back(std::move(reference));
        }
        snapshot.data().refresh();
        *result = make_handle<std::any>(std::move(snapshot));
    });
}

extern "C" int txrt_system_current_directory(void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(tx_generated::tx_fn_system_current_directory());
    });
}

extern "C" int txrt_system_set_current_directory(const void* path) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::tx_fn_system_set_current_directory(text_value(path));
    });
}

extern "C" int txrt_system_executable_path(void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(tx_generated::tx_fn_system_executable_path());
    });
}

extern "C" int txrt_system_temp_directory(void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(tx_generated::tx_fn_system_temp_directory());
    });
}

extern "C" int txrt_system_home_directory(void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(tx_generated::tx_fn_system_home_directory());
    });
}

extern "C" int txrt_system_operating_system(void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(tx_generated::tx_fn_system_operating_system());
    });
}

extern "C" int txrt_system_architecture(void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(tx_generated::tx_fn_system_architecture());
    });
}

extern "C" int txrt_system_cpu_count(std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::tx_fn_system_cpu_count();
    });
}

extern "C" int txrt_system_process_id(std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::tx_fn_system_process_id();
    });
}

extern "C" int txrt_system_has_capability(const void* name,
                                           bool* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::tx_fn_system_has_capability(text_value(name));
    });
}

extern "C" int txrt_env_contains(const void* name, bool* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::tx_fn_env_contains(text_value(name));
    });
}

extern "C" int txrt_env_get(const void* name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(tx_generated::tx_fn_env_get(text_value(name)));
    });
}

extern "C" int txrt_env_get_default(const void* name, const void* default_value,
                                   void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(tx_generated::tx_fn_env_get(
            text_value(name), text_value(default_value)));
    });
}

extern "C" int txrt_env_set(const void* name, const void* value) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::tx_fn_env_set(text_value(name), text_value(value));
    });
}

extern "C" int txrt_env_remove(const void* name, bool* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::tx_fn_env_remove(text_value(name));
    });
}

extern "C" int txrt_env_snapshot(void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::tx_fn_env_snapshot());
    });
}
