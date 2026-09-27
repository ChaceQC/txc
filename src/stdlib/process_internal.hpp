#pragma once

#include "stdlib/process.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <array>
#include <chrono>
#include <utility>

namespace tx_generated::process_detail
{

class native_handle
{
public:
    explicit native_handle(HANDLE value = nullptr) noexcept : value_(value)
    {
    }
    native_handle(const native_handle&) = delete;
    native_handle& operator=(const native_handle&) = delete;
    native_handle(native_handle&& other) noexcept
        : value_(std::exchange(other.value_, nullptr))
    {
    }
    native_handle& operator=(native_handle&& other) noexcept
    {
        reset(std::exchange(other.value_, nullptr));
        return *this;
    }
    ~native_handle()
    {
        reset();
    }
    HANDLE get() const noexcept
    {
        return value_;
    }
    bool valid() const noexcept
    {
        return value_ && value_ != INVALID_HANDLE_VALUE;
    }
    void reset(HANDLE value = nullptr) noexcept
    {
        if (valid())
        {
            CloseHandle(value_);
        }
        value_ = value;
    }
private:
    HANDLE value_;
};

struct launch_data
{
    std::wstring executable;
    std::wstring command_line;
    std::wstring cwd;
    std::vector<wchar_t> environment;
};

using steady_clock = std::chrono::steady_clock;
[[noreturn]] void fail(const char* code, const char* message, DWORD error = 0);
launch_data prepare_launch(const process_options& options);
native_handle prepare_stream(const std::string& mode, unsigned index,
    process_pipe& parent);
void require_child(const process_child& child);
void validate_timeout(std::int64_t timeout_ms);
steady_clock::time_point deadline(std::int64_t timeout_ms);
std::string cancellation_reason(const std::shared_ptr<cancellation_state>& state);
bool poll_exit(process_child_state& child);

} // namespace tx_generated::process_detail

namespace tx_generated
{

struct process_pipe_state
{
    process_detail::native_handle handle;
    bool readable = false;
    bool eof = false;
};

struct process_child_state
{
    process_detail::native_handle handle;
    DWORD id = 0;
    bool group = false;
    bool closed = false;
    bool completed = false;
    bool killed = false;
    process_status status;
    std::array<process_pipe, 3> pipes;

    ~process_child_state()
    {
        close();
    }
    void close() noexcept
    {
        closed = true;
        handle.reset();
        for (const auto& pipe : pipes)
        {
            if (pipe)
            {
                pipe->handle.reset();
            }
        }
    }
};

} // namespace tx_generated
