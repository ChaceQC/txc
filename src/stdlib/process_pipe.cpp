#include "stdlib/process_io.hpp"
#include "stdlib/process_io_internal.hpp"
#include "stdlib/bytes.hpp"

#include <algorithm>

namespace tx_generated::process_detail
{

pipe_operation::pipe_operation(process_pipe pipe) : pipe_(std::move(pipe)),
    event_(CreateEventW(nullptr, TRUE, FALSE, nullptr))
{
    if (!event_.valid())
    {
        fail("pipe_failed", "创建管道 I/O 事件失败", GetLastError());
    }
}

pipe_operation::~pipe_operation()
{
    cancel();
}

void pipe_operation::start(void* buffer, DWORD size)
{
    transferred = 0;
    error = 0;
    if (!pipe_->readable)
    {
        // PIPE_NOWAIT 保证同步调用立即返回；直接取得系统确认的短写长度。
        active_ = false;
        if (!WriteFile(pipe_->handle.get(), buffer, size, &transferred, nullptr))
        {
            error = GetLastError();
        }
        return;
    }
    ResetEvent(event_.get());
    operation_ = {};
    operation_.hEvent = event_.get();
    active_ = true;
    const auto started = ReadFile(pipe_->handle.get(), buffer, size, nullptr, &operation_);
    if (!started)
    {
        const auto code = GetLastError();
        if (code != ERROR_IO_PENDING)
        {
            active_ = false;
            error = code;
        }
    }
}

bool pipe_operation::poll() noexcept
{
    if (!active_)
    {
        return true;
    }
    if (!GetOverlappedResult(pipe_->handle.get(), &operation_, &transferred, FALSE))
    {
        error = GetLastError();
        if (error == ERROR_IO_INCOMPLETE)
        {
            error = 0;
            return false;
        }
    }
    active_ = false;
    return true;
}

void pipe_operation::cancel() noexcept
{
    if (active_)
    {
        CancelIoEx(pipe_->handle.get(), &operation_);
        if (!GetOverlappedResult(pipe_->handle.get(), &operation_, &transferred, TRUE))
        {
            error = GetLastError();
        }
        active_ = false;
    }
}

HANDLE pipe_operation::event() const noexcept
{
    return event_.get();
}

bool pipe_operation::active() const noexcept
{
    return active_;
}

bool broken_pipe(DWORD error)
{
    return error == ERROR_BROKEN_PIPE || error == ERROR_PIPE_NOT_CONNECTED ||
        error == ERROR_NO_DATA;
}

void require_pipe(const process_pipe& pipe, bool readable)
{
    if (!pipe || !pipe->handle.valid())
    {
        fail("invalid_state", "进程管道已关闭");
    }
    if (pipe->readable != readable)
    {
        fail("invalid_state", "进程管道读写方向不符");
    }
}

void wait_operation(pipe_operation& operation, steady_clock::time_point limit)
{
    while (!operation.poll())
    {
        if (steady_clock::now() >= limit)
        {
            operation.cancel();
            return;
        }
        if (WaitForSingleObject(operation.event(), 1) == WAIT_FAILED)
        {
            fail("pipe_failed", "等待管道 I/O 失败", GetLastError());
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
    process_detail::pipe_operation operation(pipe);
    operation.start(buffer.data(), static_cast<DWORD>(buffer.size()));
    process_detail::wait_operation(operation, limit);
    if (operation.error == ERROR_OPERATION_ABORTED)
    {
        return {make_bytes({}), "timeout"};
    }
    if (operation.error != 0 && !process_detail::broken_pipe(operation.error))
    {
        process_detail::fail("pipe_failed", "读取进程管道失败", operation.error);
    }
    buffer.resize(operation.transferred);
    pipe->eof = buffer.empty();
    return {make_bytes(std::move(buffer)), pipe->eof ? "eof" : "data"};
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
    process_detail::pipe_operation operation(pipe);
    do
    {
        operation.start(const_cast<std::uint8_t*>(data->data()),
            static_cast<DWORD>(std::min<std::size_t>(data->size(), 16 * 1024)));
        process_detail::wait_operation(operation, limit);
        if (process_detail::broken_pipe(operation.error))
        {
            return {operation.transferred, "closed"};
        }
        if (operation.error != 0 && operation.error != ERROR_OPERATION_ABORTED)
        {
            process_detail::fail("pipe_failed", "写入进程管道失败", operation.error);
        }
        if (operation.transferred != 0)
        {
            return {operation.transferred, "written"};
        }
        if (process_detail::steady_clock::now() >= limit)
        {
            break;
        }
        Sleep(1);
    } while (true);
    return {0, "timeout"};
}

} // namespace tx_generated
