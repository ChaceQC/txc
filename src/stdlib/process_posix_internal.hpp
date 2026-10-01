#pragma once

#include "stdlib/process.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <sys/types.h>
#include <unistd.h>
#include <utility>

namespace tx_generated::process_detail
{

class native_handle
{
public:
    explicit native_handle(int value = -1) noexcept : value_(value)
    {
    }
    native_handle(const native_handle&) = delete;
    native_handle& operator=(const native_handle&) = delete;
    native_handle(native_handle&& other) noexcept : value_(other.release())
    {
    }
    native_handle& operator=(native_handle&& other) noexcept
    {
        if (this != &other)
        {
            reset(other.release());
        }
        return *this;
    }
    ~native_handle()
    {
        reset();
    }
    int get() const noexcept
    {
        return value_;
    }
    int release() noexcept
    {
        return std::exchange(value_, -1);
    }
    bool valid() const noexcept
    {
        return value_ >= 0;
    }
    void reset(int value = -1) noexcept
    {
        if (valid())
        {
            // Linux 的 close 被信号打断时也已释放 fd，不能重试关闭复用的 fd。
            ::close(value_);
        }
        value_ = value;
    }
private:
    int value_;
};

struct launch_data
{
    std::string executable;
    std::string cwd;
    std::vector<std::string> arguments;
    std::vector<std::string> environment;
};

using steady_clock = std::chrono::steady_clock;
[[noreturn]] void fail(const char* code, const char* message, int error = 0);
launch_data prepare_launch(const process_options& options);
native_handle prepare_stream(const std::string& mode, unsigned index,
    process_pipe& parent);
void require_child(const process_child& child);
void require_pipe(const process_pipe& pipe, bool readable);
void validate_timeout(std::int64_t timeout_ms);
steady_clock::time_point deadline(std::int64_t timeout_ms);
std::string cancellation_reason(const std::shared_ptr<cancellation_state>& state);
bool poll_exit(process_child_state& child);
ssize_t write_no_signal(int descriptor, const void* data, std::size_t size);
bool wait_descriptor(int descriptor, short events, steady_clock::time_point limit);

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
    pid_t id = -1;
    bool group = false;
    bool closed = false;
    bool completed = false;
    process_status status;
    std::array<process_pipe, 3> pipes;

    ~process_child_state();
    void close() noexcept;
};

} // namespace tx_generated
