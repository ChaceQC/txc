#include "stdlib/ipc.hpp"
#include "stdlib/error.hpp"

#include <atomic>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>

namespace tx_generated
{
namespace
{

[[noreturn]] void ipc_error(const char* code, const char* message)
{
    const std::string_view value(code);
    throw runtime_failure({value == "invalid_argument" || value == "invalid_state"
        ? tx::error_kind::runtime : tx::error_kind::io, code, message});
}

void check_max(std::int64_t maximum)
{
    if (maximum < 1 || maximum > 16 * 1024 * 1024)
    {
        throw runtime_failure({tx::error_kind::runtime,
            "out_of_range", "IPC 单条消息上限必须在 1..16777216 字节内"});
    }
}

std::string socket_name(std::string_view name)
{
    if (name.empty() || name.size() > 64)
    {
        ipc_error("invalid_argument", "IPC 管道名长度必须在 1..64 内");
    }
    for (const unsigned char character : name)
    {
        if (!((character >= 'A' && character <= 'Z') ||
              (character >= 'a' && character <= 'z') ||
              (character >= '0' && character <= '9') || character == '_' || character == '-'))
        {
            ipc_error("invalid_argument", "IPC 管道名只能包含 ASCII 字母、数字、下划线和连字符");
        }
    }
    // Linux 抽象命名空间不留下磁盘节点；UID 隔离加对端凭据验证。
    return "tx-ipc-" + std::to_string(geteuid()) + '-' + std::string(name);
}

sockaddr_un socket_address(const std::string& name)
{
    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    std::memcpy(address.sun_path + 1, name.data(), name.size());
    return address;
}

socklen_t address_size(const std::string& name)
{
    return static_cast<socklen_t>(offsetof(sockaddr_un, sun_path) + 1 + name.size());
}

process_detail::native_handle new_socket()
{
    process_detail::native_handle result(::socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0));
    if (!result.valid())
    {
        ipc_error("ipc_connect_failed", "无法创建本机 IPC 套接字");
    }
    return result;
}

ipc_stream connected(process_detail::native_handle handle, std::size_t maximum)
{
    ucred credentials{};
    socklen_t length = sizeof(credentials);
    if (getsockopt(handle.get(), SOL_SOCKET, SO_PEERCRED, &credentials, &length) < 0 ||
        credentials.uid != geteuid())
    {
        ipc_error("ipc_connect_failed", "IPC 对端不属于当前用户");
    }
    static std::atomic<std::uint64_t> serial = 1;
    auto result = std::make_shared<ipc_stream_state>();
    result->native = std::move(handle);
    result->max_bytes = maximum;
    result->local_session = static_cast<std::int64_t>(
        (static_cast<std::uint64_t>(getpid()) << 32) ^
        static_cast<std::uint64_t>(ipc_clock::now().time_since_epoch().count()) ^ serial.fetch_add(1));
    return result;
}

void check_wait(ipc_clock::time_point deadline, const std::shared_ptr<cancellation_state>& token)
{
    if (ipc_cancelled(token))
    {
        throw runtime_failure({tx::error_kind::cancelled, "cancelled", "IPC 操作已取消"});
    }
    if (ipc_clock::now() >= deadline)
    {
        throw runtime_failure({tx::error_kind::runtime, "timeout", "等待 IPC 连接超时"});
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
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
    const auto now = ipc_clock::now();
    const auto available = std::chrono::duration_cast<std::chrono::milliseconds>(
        ipc_clock::time_point::max() - now).count();
    if (timeout_ms < -1 || timeout_ms > available)
    {
        throw runtime_failure({tx::error_kind::runtime, "invalid_argument", "IPC 超时超出单调时钟范围"});
    }
    return timeout_ms == -1 ? ipc_clock::time_point::max() : now + std::chrono::milliseconds(timeout_ms);
}

bool ipc_cancelled(const std::shared_ptr<cancellation_state>& token)
{
    return !process_detail::cancellation_reason(token).empty();
}

ipc_listener ipc_listen(std::string_view name, std::int64_t max_bytes)
{
    check_max(max_bytes);
    auto result = std::make_shared<ipc_listener_state>();
    result->name = socket_name(name);
    result->max_bytes = static_cast<std::size_t>(max_bytes);
    result->pending = new_socket();
    const auto address = socket_address(result->name);
    if (::bind(result->pending.get(), reinterpret_cast<const sockaddr*>(&address), address_size(result->name)) < 0 ||
        ::listen(result->pending.get(), SOMAXCONN) < 0)
    {
        ipc_error("ipc_listen_failed", "无法监听本机 IPC 套接字");
    }
    return result;
}

ipc_stream ipc_accept(const ipc_listener& listener, std::int64_t timeout_ms,
    const std::shared_ptr<cancellation_state>& token)
{
    const auto deadline = ipc_deadline(timeout_ms);
    for (;;)
    {
        if (ipc_cancelled(token))
        {
            check_wait(deadline, token);
        }
        {
            std::lock_guard lock(listener->mutex);
            if (listener->closed)
            {
                ipc_error("invalid_state", "IPC 监听器已关闭");
            }
            process_detail::native_handle accepted(::accept4(listener->pending.get(), nullptr, nullptr,
                SOCK_NONBLOCK | SOCK_CLOEXEC));
            if (accepted.valid())
            {
                return connected(std::move(accepted), listener->max_bytes);
            }
            if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
            {
                ipc_error("ipc_accept_failed", "IPC 连接等待失败");
            }
        }
        check_wait(deadline, token);
    }
}

ipc_stream ipc_connect(std::string_view name, std::int64_t max_bytes,
    std::int64_t timeout_ms, const std::shared_ptr<cancellation_state>& token)
{
    check_max(max_bytes);
    const auto native_name = socket_name(name);
    const auto address = socket_address(native_name);
    const auto deadline = ipc_deadline(timeout_ms);
    for (;;)
    {
        if (ipc_cancelled(token))
        {
            check_wait(deadline, token);
        }
        auto handle = new_socket();
        if (::connect(handle.get(), reinterpret_cast<const sockaddr*>(&address), address_size(native_name)) == 0)
        {
            return connected(std::move(handle), static_cast<std::size_t>(max_bytes));
        }
        if (errno != ECONNREFUSED && errno != ENOENT && errno != EAGAIN && errno != EINTR)
        {
            ipc_error("ipc_connect_failed", "无法连接本机 IPC 套接字");
        }
        check_wait(deadline, token);
    }
}

ipc_stream ipc_from_process_pipes(const process_pipe& reader, const process_pipe& writer,
    std::int64_t max_bytes)
{
    check_max(max_bytes);
    if (!reader || !writer || !reader->handle.valid() || !writer->handle.valid() ||
        !reader->readable || writer->readable)
    {
        ipc_error("invalid_argument", "进程 IPC 需要可读 stdout 管道和可写 stdin 管道");
    }
    auto result = std::make_shared<ipc_stream_state>();
    result->reader = reader;
    result->writer = writer;
    result->max_bytes = static_cast<std::size_t>(max_bytes);
    result->local_session = static_cast<std::int64_t>(ipc_clock::now().time_since_epoch().count());
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
