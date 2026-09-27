#include "backend/cpp/process_abi.hpp"
#include "backend/cpp/process_abi_internal.hpp"
#include "stdlib/vector.hpp"

namespace tx_generated::process_abi
{
namespace
{

std::vector<std::string> strings(const string_vector& source)
{
    std::vector<std::string> result;
    result.reserve(source.data().values.size());
    for (const auto& item : source.data().values)
    {
        result.push_back(item.get());
    }
    return result;
}

} // namespace

process_options options(const void* handle)
{
    const auto& object = value<dynamic_struct>(handle);
    process_options result;
    result.executable = field<std::string>(object, 0);
    result.args = strings(field<string_vector>(object, 1));
    result.cwd = field<std::string>(object, 2);
    result.env = strings(field<string_vector>(object, 3));
    result.clear_env = field<bool>(object, 4);
    result.stdin_mode = field<std::string>(object, 5);
    result.stdout_mode = field<std::string>(object, 6);
    result.stderr_mode = field<std::string>(object, 7);
    result.new_process_group = field<bool>(object, 8);
    return result;
}

dynamic_struct status(const char* type_name, process_status value)
{
    struct_fields fields(2);
    fields[0] = {"state", std::move(value.state)};
    fields[1] = {"exit_code", value.exit_code};
    return dynamic_struct(dynamic_struct_data{type_name, "exit_status", std::move(fields)});
}

} // namespace tx_generated::process_abi

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;
using tx_generated::process_abi::value;
using tx_generated::process_child;

extern "C" int txrt_process_make_options(const void* executable, const void* args,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::struct_fields fields(9);
        fields[0] = {"executable", *static_cast<const std::string*>(executable)};
        fields[1] = {"args", value<tx_generated::string_vector>(args).copy()};
        fields[2] = {"cwd", std::string{}};
        fields[3] = {"env", tx_generated::string_vector{}};
        fields[4] = {"clear_env", false};
        fields[5] = {"stdin_mode", std::string("pipe")};
        fields[6] = {"stdout_mode", std::string("pipe")};
        fields[7] = {"stderr_mode", std::string("pipe")};
        fields[8] = {"new_process_group", false};
        *result = make_handle<std::any>(tx_generated::dynamic_struct(
            tx_generated::dynamic_struct_data{type_name, "options", std::move(fields)}));
    }, tx::error_kind::process);
}

extern "C" int txrt_process_spawn(const void* config, void** result) noexcept
{
    return invoke_checked([&]
    {
        // 先分配 TX 外壳；启动之后不再因分配外壳而丢失已启动的子进程。
        auto* handle = make_handle<std::any>(process_child{});
        try
        {
            std::any_cast<process_child&>(*handle) =
                tx_generated::process_spawn(tx_generated::process_abi::options(config));
        }
        catch (...)
        {
            tx_generated::detail::destroy_handle(handle);
            throw;
        }
        *result = handle;
    }, tx::error_kind::process);
}

extern "C" int txrt_process_id(const void* child, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::process_id(value<process_child>(child));
    }, tx::error_kind::process);
}

extern "C" int txrt_process_wait(const void* child, std::int64_t timeout_ms,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::process_abi::status(type_name,
            tx_generated::process_wait(value<process_child>(child), timeout_ms)));
    }, tx::error_kind::process);
}

extern "C" int txrt_process_wait_with_cancel(const void* child,
    std::int64_t timeout_ms, const void* token, const char* type_name,
    void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::process_abi::status(type_name,
            tx_generated::process_wait(value<process_child>(child), timeout_ms,
                value<tx_generated::cancel_token>(token).state)));
    }, tx::error_kind::process);
}

extern "C" int txrt_process_try_wait(const void* child, const char* type_name,
    void** result) noexcept
{
    return txrt_process_wait(child, 0, type_name, result);
}

extern "C" int txrt_process_terminate(const void* child) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::process_terminate(value<process_child>(child));
    }, tx::error_kind::process);
}

extern "C" int txrt_process_kill(const void* child) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::process_kill(value<process_child>(child));
    }, tx::error_kind::process);
}

extern "C" int txrt_process_close(const void* child) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::process_close(value<process_child>(child));
    }, tx::error_kind::process);
}
