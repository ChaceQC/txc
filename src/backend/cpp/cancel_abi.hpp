#pragma once

#include <cstdint>

extern "C"
{

int txrt_cancel_source(void** result) noexcept;
int txrt_cancel_with_deadline_ms(std::int64_t timeout_ms,
    void** result) noexcept;
int txrt_cancel_token(const void* owner, void** result) noexcept;
int txrt_cancel_cancel(const void* owner, bool* result) noexcept;
int txrt_cancel_status(const void* value,
    std::int64_t* result) noexcept;
int txrt_cancel_wait(const void* value, std::int64_t timeout_ms,
    std::int64_t* result) noexcept;

}
