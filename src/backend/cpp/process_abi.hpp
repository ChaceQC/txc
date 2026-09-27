#pragma once

#include <cstdint>

extern "C"
{
int txrt_process_make_options(const void* executable, const void* args,
    const char* type_name, void** result) noexcept;
int txrt_process_spawn(const void* config, void** result) noexcept;
int txrt_process_id(const void* child, std::int64_t* result) noexcept;
int txrt_process_wait(const void* child, std::int64_t timeout_ms,
    const char* type_name, void** result) noexcept;
int txrt_process_wait_with_cancel(const void* child, std::int64_t timeout_ms,
    const void* token, const char* type_name, void** result) noexcept;
int txrt_process_try_wait(const void* child, const char* type_name,
    void** result) noexcept;
int txrt_process_terminate(const void* child) noexcept;
int txrt_process_kill(const void* child) noexcept;
int txrt_process_close(const void* child) noexcept;
}
