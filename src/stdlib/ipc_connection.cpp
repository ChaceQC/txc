#include "stdlib/ipc.hpp"
#include "stdlib/error.hpp"

#include <atomic>
#include <limits>

namespace tx_generated
{
namespace
{

void check_max(std::int64_t max_bytes)
{
    if (max_bytes < 1 || max_bytes > 16 * 1024 * 1024)
    {
        throw runtime_failure({tx::error_kind::runtime,
            "out_of_range", "IPC 单条消息上限必须在 1..16777216 字节内"});
    }
}

std::wstring pipe_name(std::string_view name)
{
    if (name.empty() || name.size() > 64)
    {
        throw runtime_failure({tx::error_kind::runtime,
            "invalid_argument", "IPC 管道名长度必须在 1..64 内"});
    }
    for (const unsigned char character : name)
    {
        const bool letter = (character >= 'A' && character <= 'Z') ||
            (character >= 'a' && character <= 'z');
        const bool digit = character >= '0' && character <= '9';
        if (!letter && !digit && character != '_' && character != '-')
        {
            throw runtime_failure({tx::error_kind::runtime,
                "invalid_argument", "IPC 管道名只能包含 ASCII 字母、数字、下划线和连字符"});
        }
    }
    return L"\\\\.\\pipe\\tx-ipc-" +
        std::wstring(name.begin(), name.end());
}

std::int64_t new_session() noexcept
{
    static std::atomic<std::uint64_t> serial = 1;
    LARGE_INTEGER counter{};
    QueryPerformanceCounter(&counter);
    const auto value = (static_cast<std::uint64_t>(GetCurrentProcessId()) << 32) ^
        static_cast<std::uint64_t>(counter.QuadPart) ^
        serial.fetch_add(1, std::memory_order_relaxed);
    return static_cast<std::int64_t>(value);
}

HANDLE new_server_pipe(const std::wstring& name, bool first)
{
    const auto handle = CreateNamedPipeW(name.c_str(),
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED |
            (first ? FILE_FLAG_FIRST_PIPE_INSTANCE : 0),
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT |
            PIPE_REJECT_REMOTE_CLIENTS,
        PIPE_UNLIMITED_INSTANCES, 64 * 1024, 64 * 1024, 0, nullptr);
    if (handle == INVALID_HANDLE_VALUE)
    {
        throw runtime_failure({tx::error_kind::io,
            "ipc_listen_failed", "无法创建本机 IPC 管道"});
    }
    return handle;
}

bool wait_connection(HANDLE pipe, ipc_clock::time_point deadline,
                     const std::shared_ptr<cancellation_state>& token)
{
    process_detail::native_handle event(CreateEventW(
        nullptr, TRUE, FALSE, nullptr));
    if (!event.valid())
    {
        throw runtime_failure({tx::error_kind::io,
            "ipc_accept_failed", "无法创建 IPC 连接事件"});
    }
    OVERLAPPED operation{};
    operation.hEvent = event.get();
    if (ConnectNamedPipe(pipe, &operation))
    {
        return true;
    }
    const auto started = GetLastError();
    if (started == ERROR_PIPE_CONNECTED)
    {
        return true;
    }
    if (started != ERROR_IO_PENDING)
    {
        throw runtime_failure({tx::error_kind::io,
            "ipc_accept_failed", "无法等待 IPC 客户端连接"});
    }
    for (;;)
    {
        if (ipc_cancelled(token) || ipc_clock::now() >= deadline)
        {
            CancelIoEx(pipe, &operation);
            DWORD ignored = 0;
            GetOverlappedResult(pipe, &operation, &ignored, TRUE);
            return false;
        }
        const auto status = WaitForSingleObject(event.get(), 10);
        if (status == WAIT_OBJECT_0)
        {
            DWORD ignored = 0;
            if (GetOverlappedResult(pipe, &operation, &ignored, FALSE))
            {
                return true;
            }
            throw runtime_failure({tx::error_kind::io,
                "ipc_accept_failed", "IPC 连接等待失败"});
        }
        if (status == WAIT_FAILED)
        {
            CancelIoEx(pipe, &operation);
            DWORD ignored = 0;
            GetOverlappedResult(pipe, &operation, &ignored, TRUE);
            throw runtime_failure({tx::error_kind::io,
                "ipc_accept_failed", "IPC 连接事件等待失败"});
        }
    }
}

} // namespace

ipc_stream_state::~ipc_stream_state()
{
    native.reset();
    if (reader)
    {
        reader->handle.reset();
    }
    if (writer)
    {
        writer->handle.reset();
    }
}

ipc_clock::time_point ipc_deadline(std::int64_t timeout_ms)
{
    if (timeout_ms < -1)
    {
        throw runtime_failure({tx::error_kind::runtime,
            "invalid_argument", "IPC 超时只能为 -1 或非负毫秒"});
    }
    if (timeout_ms == -1)
    {
        return ipc_clock::time_point::max();
    }
    const auto now = ipc_clock::now();
    const auto available = std::chrono::duration_cast<
        std::chrono::milliseconds>(ipc_clock::time_point::max() - now).count();
    if (timeout_ms > available)
    {
        throw runtime_failure({tx::error_kind::runtime,
            "out_of_range", "IPC 超时超出单调时钟范围"});
    }
    return now + std::chrono::milliseconds(timeout_ms);
}

bool ipc_cancelled(const std::shared_ptr<cancellation_state>& token)
{
    std::lock_guard lock(token->mutex);
    return token->cancelled ||
        (token->deadline && ipc_clock::now() >= *token->deadline);
}

ipc_listener ipc_listen(std::string_view name, std::int64_t max_bytes)
{
    check_max(max_bytes);
    auto result = std::make_shared<ipc_listener_state>();
    result->name = pipe_name(name);
    result->max_bytes = static_cast<std::size_t>(max_bytes);
    result->pending.reset(new_server_pipe(result->name, true));
    return result;
}

ipc_stream ipc_accept(const ipc_listener& listener, std::int64_t timeout_ms,
                      const std::shared_ptr<cancellation_state>& token)
{
    const auto deadline = ipc_deadline(timeout_ms);
    process_detail::native_handle pending;
    {
        std::lock_guard lock(listener->mutex);
        if (listener->closed || !listener->pending.valid())
        {
            throw runtime_failure({tx::error_kind::runtime,
                "invalid_state", "IPC 监听器已关闭"});
        }
        pending = std::move(listener->pending);
    }
    bool connected = false;
    try
    {
        connected = wait_connection(pending.get(), deadline, token);
    }
    catch (...)
    {
        std::lock_guard lock(listener->mutex);
        listener->pending.reset(new_server_pipe(listener->name, false));
        throw;
    }
    {
        std::lock_guard lock(listener->mutex);
        listener->pending.reset(new_server_pipe(listener->name, false));
    }
    if (!connected)
    {
        throw runtime_failure({ipc_cancelled(token)
            ? tx::error_kind::cancelled : tx::error_kind::runtime,
            ipc_cancelled(token) ? "cancelled" : "timeout",
            "等待 IPC 客户端连接已取消或超时"});
    }
    auto result = std::make_shared<ipc_stream_state>();
    result->native = std::move(pending);
    result->max_bytes = listener->max_bytes;
    result->local_session = new_session();
    return result;
}

ipc_stream ipc_connect(std::string_view name, std::int64_t max_bytes,
                       std::int64_t timeout_ms,
                       const std::shared_ptr<cancellation_state>& token)
{
    check_max(max_bytes);
    const auto native_name = pipe_name(name);
    const auto deadline = ipc_deadline(timeout_ms);
    for (;;)
    {
        if (ipc_cancelled(token))
        {
            throw runtime_failure({tx::error_kind::cancelled,
                "cancelled", "IPC 连接已取消"});
        }
        process_detail::native_handle handle(CreateFileW(
            native_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0,
            nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr));
        if (handle.valid())
        {
            auto result = std::make_shared<ipc_stream_state>();
            result->native = std::move(handle);
            result->max_bytes = static_cast<std::size_t>(max_bytes);
            result->local_session = new_session();
            return result;
        }
        const auto error = GetLastError();
        if (error != ERROR_PIPE_BUSY && error != ERROR_FILE_NOT_FOUND)
        {
            throw runtime_failure({tx::error_kind::io,
                "ipc_connect_failed", "无法连接本机 IPC 管道"});
        }
        if (ipc_clock::now() >= deadline)
        {
            throw runtime_failure({tx::error_kind::runtime,
                "timeout", "等待 IPC 管道连接超时"});
        }
        if (error == ERROR_PIPE_BUSY)
        {
            WaitNamedPipeW(native_name.c_str(), 10);
        }
        else
        {
            Sleep(10);
        }
    }
}

ipc_stream ipc_from_process_pipes(const process_pipe& reader,
    const process_pipe& writer, std::int64_t max_bytes)
{
    check_max(max_bytes);
    if (!reader || !writer || !reader->handle.valid() ||
        !writer->handle.valid() || !reader->readable || writer->readable)
    {
        throw runtime_failure({tx::error_kind::runtime,
            "invalid_argument", "进程 IPC 需要可读 stdout 管道和可写 stdin 管道"});
    }
    auto result = std::make_shared<ipc_stream_state>();
    result->reader = reader;
    result->writer = writer;
    result->max_bytes = static_cast<std::size_t>(max_bytes);
    result->local_session = new_session();
    return result;
}

bool ipc_close_listener(const ipc_listener& listener)
{
    std::lock_guard lock(listener->mutex);
    const bool changed = !listener->closed;
    listener->closed = true;
    listener->pending.reset();
    return changed;
}

bool ipc_close(const ipc_stream& stream)
{
    const bool changed = !stream->closed;
    stream->closed = true;
    stream->native.reset();
    if (stream->reader)
    {
        stream->reader->handle.reset();
    }
    if (stream->writer)
    {
        stream->writer->handle.reset();
    }
    stream->pending.clear();
    return changed;
}

} // namespace tx_generated
