#pragma once

#include "stdlib/process_internal.hpp"

namespace tx_generated::process_detail
{

// OVERLAPPED 和缓冲必须活到系统请求完成；析构总是取消并收尾。
class pipe_operation
{
public:
    explicit pipe_operation(process_pipe pipe);
    pipe_operation(const pipe_operation&) = delete;
    pipe_operation& operator=(const pipe_operation&) = delete;
    ~pipe_operation();
    void start(void* buffer, DWORD size);
    bool poll() noexcept;
    void cancel() noexcept;
    HANDLE event() const noexcept;
    bool active() const noexcept;
    DWORD transferred = 0;
    DWORD error = 0;
private:
    process_pipe pipe_;
    native_handle event_;
    OVERLAPPED operation_{};
    bool active_ = false;
};

bool broken_pipe(DWORD error);
void require_pipe(const process_pipe& pipe, bool readable);
void wait_operation(pipe_operation& operation, steady_clock::time_point limit);

} // namespace tx_generated::process_detail
