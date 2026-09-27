#include "backend/cpp/process_io_abi.hpp"
#include "backend/cpp/process_abi_internal.hpp"
#include "stdlib/process_io.hpp"
#include "stdlib/bytes.hpp"

namespace
{

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;
using tx_generated::process_abi::value;
using tx_generated::process_abi::field;
using tx_generated::dynamic_struct;
using tx_generated::dynamic_struct_data;
using tx_generated::process_child;
using tx_generated::process_pipe;
using tx_generated::struct_fields;

int get_pipe(const void* child, unsigned index, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::process_get_pipe(
            value<process_child>(child), index));
    }, tx::error_kind::process);
}

tx_generated::process_limits run_limits(const void* handle)
{
    const auto& object = value<dynamic_struct>(handle);
    return {field<tx_generated::byte_value>(object, 0),
        field<std::int64_t>(object, 1), field<std::int64_t>(object, 2),
        field<std::string>(object, 3)};
}

void* completed(const char* type_name, const char* status_type,
    tx_generated::process_completed result)
{
    struct_fields fields(6);
    fields[0] = {"child", std::move(result.child)};
    fields[1] = {"status", tx_generated::process_abi::status(status_type, std::move(result.status))};
    fields[2] = {"stdout", std::move(result.stdout_data)};
    fields[3] = {"stderr", std::move(result.stderr_data)};
    fields[4] = {"reason", std::move(result.reason)};
    fields[5] = {"input_written", result.input_written};
    return make_handle<std::any>(dynamic_struct(dynamic_struct_data{
        type_name, "completed", std::move(fields)}));
}

} // namespace

extern "C" int txrt_process_stdin_pipe(const void* child, void** result) noexcept
{
    return get_pipe(child, 0, result);
}

extern "C" int txrt_process_stdout_pipe(const void* child, void** result) noexcept
{
    return get_pipe(child, 1, result);
}

extern "C" int txrt_process_stderr_pipe(const void* child, void** result) noexcept
{
    return get_pipe(child, 2, result);
}

extern "C" int txrt_process_close_pipe(const void* pipe) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::process_close_pipe(value<process_pipe>(pipe));
    }, tx::error_kind::process);
}

extern "C" int txrt_process_read_pipe(const void* pipe, std::int64_t max_bytes,
    std::int64_t timeout_ms, const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        auto chunk = tx_generated::process_read_pipe(value<process_pipe>(pipe),
            max_bytes, timeout_ms);
        struct_fields fields(2);
        fields[0] = {"data", std::move(chunk.data)};
        fields[1] = {"state", std::move(chunk.state)};
        *result = make_handle<std::any>(dynamic_struct(dynamic_struct_data{
            type_name, "pipe_chunk", std::move(fields)}));
    }, tx::error_kind::process);
}

extern "C" int txrt_process_write_pipe(const void* pipe, const void* data,
    std::int64_t timeout_ms, const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto written = tx_generated::process_write_pipe(value<process_pipe>(pipe),
            value<tx_generated::byte_value>(data), timeout_ms);
        struct_fields fields(2);
        fields[0] = {"written", written.written};
        fields[1] = {"state", written.state};
        *result = make_handle<std::any>(dynamic_struct(dynamic_struct_data{
            type_name, "pipe_write", std::move(fields)}));
    }, tx::error_kind::process);
}

extern "C" int txrt_process_make_limits(const void* input, std::int64_t timeout_ms,
    std::int64_t max_output_bytes, const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        struct_fields fields(4);
        fields[0] = {"input", value<tx_generated::byte_value>(input)};
        fields[1] = {"timeout_ms", timeout_ms};
        fields[2] = {"max_output_bytes", max_output_bytes};
        fields[3] = {"stop_action", std::string("keep")};
        *result = make_handle<std::any>(dynamic_struct(dynamic_struct_data{
            type_name, "run_limits", std::move(fields)}));
    }, tx::error_kind::process);
}

extern "C" int txrt_process_run(const void* config, const void* limits,
    const char* type_name, const char* status_type, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = completed(type_name, status_type, tx_generated::process_run(
            tx_generated::process_abi::options(config), run_limits(limits)));
    }, tx::error_kind::process);
}

extern "C" int txrt_process_run_with_cancel(const void* config, const void* limits,
    const void* token, const char* type_name, const char* status_type,
    void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = completed(type_name, status_type, tx_generated::process_run(
            tx_generated::process_abi::options(config), run_limits(limits),
            value<tx_generated::cancel_token>(token).state));
    }, tx::error_kind::process);
}
