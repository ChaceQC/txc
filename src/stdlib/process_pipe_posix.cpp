#include "stdlib/process_internal.hpp"
#include "stdlib/process_io.hpp"
#include "stdlib/bytes.hpp"

#include <algorithm>
#include <cerrno>
#include <csignal>
#include <poll.h>
#include <pthread.h>

namespace tx_generated::process_detail
{

ssize_t write_no_signal(int descriptor, const void* data, std::size_t size)
{
    // 只屏蔽当前线程本次写入产生的 SIGPIPE，不改变宿主的全局信号策略。
    sigset_t mask, previous, pending;
    sigemptyset(&mask);
    sigaddset(&mask, SIGPIPE);
    const int mask_error = pthread_sigmask(SIG_BLOCK, &mask, &previous);
    if (mask_error)
    {
        errno = mask_error;
        return -1;
    }
    sigpending(&pending);
    const bool had_signal = sigismember(&pending, SIGPIPE);
    ssize_t result;
    do
    {
        result = ::write(descriptor, data, size);
    } while (result < 0 && errno == EINTR);
    const int error = errno;
    if (result < 0 && error == EPIPE && !had_signal)
    {
        const timespec immediate{};
        while (sigtimedwait(&mask, nullptr, &immediate) < 0 && errno == EINTR)
        {
        }
    }
    pthread_sigmask(SIG_SETMASK, &previous, nullptr);
    errno = error;
    return result;
}

void require_pipe(const process_pipe& pipe, bool readable)
{
    if (!pipe || !pipe->handle.valid() || pipe->readable != readable)
    {
        fail("invalid_state", "进程管道已关闭或读写方向不符");
    }
}

bool wait_descriptor(int descriptor, short events, steady_clock::time_point limit)
{
    for (;;)
    {
        const auto now = steady_clock::now();
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(limit - now).count();
        const int timeout = limit == steady_clock::time_point::max() ? -1
            : static_cast<int>(std::clamp<std::int64_t>(remaining, 0, 1000));
        pollfd item{descriptor, events, 0};
        const int result = ::poll(&item, 1, timeout);
        if (result > 0)
        {
            if (item.revents & POLLNVAL)
            {
                fail("pipe_failed", "进程管道已失效", EBADF);
            }
            return true;
        }
        if (result < 0 && errno != EINTR)
        {
            fail("pipe_failed", "等待进程管道失败", errno);
        }
        if (steady_clock::now() >= limit)
        {
            return false;
        }
    }
}

} // namespace tx_generated::process_detail

namespace tx_generated
{

process_pipe process_get_pipe(const process_child& child, unsigned index)
{
    process_detail::require_child(child);
    if (index >= child->pipes.size() || !child->pipes[index])
    {
        process_detail::fail("invalid_state", "该标准流未配置为管道");
    }
    return child->pipes[index];
}

void process_close_pipe(const process_pipe& pipe)
{
    if (pipe)
    {
        pipe->handle.reset();
    }
}

process_chunk process_read_pipe(const process_pipe& pipe, std::int64_t max_bytes,
    std::int64_t timeout_ms)
{
    process_detail::require_pipe(pipe, true);
    const auto limit = process_detail::deadline(timeout_ms);
    if (max_bytes <= 0 || max_bytes > 16 * 1024 * 1024)
    {
        process_detail::fail("invalid_argument", "管道单次读取大小必须为 1～16 MiB");
    }
    if (pipe->eof)
    {
        return {make_bytes({}), "eof"};
    }
    std::vector<std::uint8_t> buffer(static_cast<std::size_t>(max_bytes));
    for (;;)
    {
        const auto size = ::read(pipe->handle.get(), buffer.data(), buffer.size());
        if (size >= 0)
        {
            buffer.resize(static_cast<std::size_t>(size));
            pipe->eof = size == 0;
            return {make_bytes(std::move(buffer)), size ? "data" : "eof"};
        }
        if (errno == EINTR)
        {
            continue;
        }
        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            process_detail::fail("pipe_failed", "读取进程管道失败", errno);
        }
        if (!process_detail::wait_descriptor(pipe->handle.get(), POLLIN, limit))
        {
            return {make_bytes({}), "timeout"};
        }
    }
}

process_write_result process_write_pipe(const process_pipe& pipe,
    const byte_value& data, std::int64_t timeout_ms)
{
    process_detail::require_pipe(pipe, false);
    const auto limit = process_detail::deadline(timeout_ms);
    if (!data || data->size() > 16 * 1024 * 1024)
    {
        process_detail::fail("invalid_argument", "管道单次写入不能超过 16 MiB");
    }
    if (data->empty())
    {
        return {0, "written"};
    }
    for (;;)
    {
        const auto size = process_detail::write_no_signal(pipe->handle.get(),
            data->data(), std::min<std::size_t>(data->size(), 16 * 1024));
        if (size >= 0)
        {
            return {size, "written"};
        }
        if (errno == EPIPE)
        {
            return {0, "closed"};
        }
        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            process_detail::fail("pipe_failed", "写入进程管道失败", errno);
        }
        if (!process_detail::wait_descriptor(pipe->handle.get(), POLLOUT, limit))
        {
            return {0, "timeout"};
        }
    }
}

} // namespace tx_generated
