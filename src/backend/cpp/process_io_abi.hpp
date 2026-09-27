#pragma once

#include <cstdint>

extern "C"
{
int txrt_process_stdin_pipe(const void* child, void** result) noexcept;
int txrt_process_stdout_pipe(const void* child, void** result) noexcept;
int txrt_process_stderr_pipe(const void* child, void** result) noexcept;
int txrt_process_close_pipe(const void* pipe) noexcept;
int txrt_process_read_pipe(const void* pipe, std::int64_t max_bytes,
    std::int64_t timeout_ms, const char* type_name, void** result) noexcept;
int txrt_process_write_pipe(const void* pipe, const void* data,
    std::int64_t timeout_ms, const char* type_name, void** result) noexcept;
int txrt_process_make_limits(const void* input, std::int64_t timeout_ms,
    std::int64_t max_output_bytes, const char* type_name, void** result) noexcept;
int txrt_process_run(const void* config, const void* limits,
    const char* type_name, const char* status_type, void** result) noexcept;
int txrt_process_run_with_cancel(const void* config, const void* limits,
    const void* token, const char* type_name, const char* status_type,
    void** result) noexcept;
}
