#include "stdlib/socket.hpp"

#include "stdlib/error.hpp"

#include <algorithm>
#include <chrono>
#include <iterator>
#include <limits>
#include <unordered_map>

namespace tx_generated::socket
{
namespace
{

struct registry
{
    std::mutex mutex;
    std::int64_t next_id = 1;
    std::unordered_map<std::int64_t, std::weak_ptr<state>> values;
};

registry& records()
{
    static registry value;
    return value;
}

[[noreturn]] void closed_error(resource_kind kind)
{
    network::fail(kind == resource_kind::tcp_stream
        ? "connection_closed" : "closed_handle", "网络句柄已关闭");
}

} // namespace

resource register_socket(resource_kind kind, network::socket_handle native)
{
    auto value = std::make_shared<state>();
    value->kind = kind;
    value->native = std::make_shared<network::socket_handle>(std::move(native));
    auto& registry = records();
    std::lock_guard lock(registry.mutex);
    if (registry.next_id == std::numeric_limits<std::int64_t>::max())
    {
        network::fail("size_limit", "网络句柄编号已耗尽");
    }
    const auto id = registry.next_id++;
    if (id % 256 == 0)
    {
        for (auto item = registry.values.begin();
             item != registry.values.end();)
        {
            item = item->second.expired()
                ? registry.values.erase(item) : std::next(item);
        }
    }
    registry.values.emplace(id, value);
    return {id, std::move(value)};
}

std::shared_ptr<state> get(std::int64_t id, resource_kind kind)
{
    std::shared_ptr<state> value;
    {
        auto& registry = records();
        std::lock_guard lock(registry.mutex);
        const auto found = registry.values.find(id);
        if (found != registry.values.end())
        {
            value = found->second.lock();
        }
    }
    if (!value || value->kind != kind || value->closed)
    {
        closed_error(kind);
    }
    return value;
}

void close(std::int64_t id, resource_kind kind)
{
    std::shared_ptr<state> value;
    {
        auto& registry = records();
        std::lock_guard lock(registry.mutex);
        const auto found = registry.values.find(id);
        if (found == registry.values.end())
        {
            return;
        }
        value = found->second.lock();
        if (value && value->kind != kind)
        {
            network::fail("invalid_argument", "网络句柄类型不匹配");
        }
        registry.values.erase(found);
    }
    if (value)
    {
        value->closed = true;
        std::lock_guard lock(value->mutex);
        value->native.reset();
    }
}

std::shared_ptr<network::socket_handle> native_socket(
    const std::shared_ptr<state>& value)
{
    std::lock_guard lock(value->mutex);
    if (value->closed || !value->native)
    {
        closed_error(value->kind);
    }
    return value->native;
}

std::shared_ptr<network::socket_handle> take_tcp(std::int64_t id)
{
    auto value = get(id, resource_kind::tcp_stream);
    std::scoped_lock io_lock(value->read_mutex, value->write_mutex);
    std::shared_ptr<network::socket_handle> native;
    {
        std::lock_guard lock(value->mutex);
        if (value->closed || !value->native)
        {
            closed_error(value->kind);
        }
        value->closed = true;
        native = std::move(value->native);
    }
    auto& registry = records();
    std::lock_guard lock(registry.mutex);
    registry.values.erase(id);
    return native;
}

void check_timeout(std::int64_t timeout_ms)
{
    if (timeout_ms < 1 || timeout_ms > 60000)
    {
        network::fail("invalid_argument", "网络超时必须为 1～60000 毫秒");
    }
}

void check_port(std::int64_t port, bool allow_zero)
{
    if (port < (allow_zero ? 0 : 1) || port > 65535)
    {
        network::fail("invalid_argument", "网络端口无效");
    }
}

address_list numeric_addresses(std::string_view ip, std::int64_t port,
                               int socktype, int protocol, bool passive)
{
    network::initialize_winsock();
    network::validate_utf8(ip);
    if ((!passive && ip.empty()) || ip.find('\0') != std::string_view::npos)
    {
        network::fail("invalid_argument", "数值 IP 地址为空或包含 NUL");
    }
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = socktype;
    hints.ai_protocol = protocol;
    hints.ai_flags = AI_NUMERICSERV | (passive ? AI_PASSIVE : AI_NUMERICHOST);
    if (!ip.empty())
    {
        hints.ai_flags |= AI_NUMERICHOST;
    }
    addrinfo* result = nullptr;
    const auto service = std::to_string(port);
    const std::string address(ip);
    if (getaddrinfo(ip.empty() ? nullptr : address.c_str(), service.c_str(),
            &hints, &result) != 0)
    {
        network::fail("invalid_argument", "数值 IP 地址无效");
    }
    return address_list(result, freeaddrinfo);
}

network::socket_handle create_socket(const addrinfo& address)
{
    network::socket_handle result(WSASocketW(address.ai_family,
        address.ai_socktype, address.ai_protocol, nullptr, 0,
        WSA_FLAG_OVERLAPPED));
    if (!result.valid())
    {
        socket_error("创建 socket");
    }
    u_long nonblocking = 1;
    if (ioctlsocket(result.get(), FIONBIO, &nonblocking) != 0)
    {
        socket_error("设置非阻塞模式");
    }
    return result;
}

void wait_ready(const std::shared_ptr<state>& value, SOCKET native,
                bool write, std::chrono::steady_clock::time_point deadline,
                const cancellation_probe& cancellation)
{
    while (true)
    {
        if (value->closed)
        {
            closed_error(value->kind);
        }
        if (cancellation)
        {
            cancellation();
        }
        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline)
        {
            network::fail("timeout", "等待网络操作超时");
        }
        const auto remaining = std::chrono::duration_cast<
            std::chrono::milliseconds>(deadline - now).count();
        const auto slice = std::min<std::int64_t>(remaining + 1, 10);
        fd_set ready;
        FD_ZERO(&ready);
        FD_SET(native, &ready);
        timeval limit{0, static_cast<long>(slice * 1000)};
        const int selected = select(0, write ? nullptr : &ready,
            write ? &ready : nullptr, nullptr, &limit);
        if (selected > 0)
        {
            if (value->closed)
            {
                closed_error(value->kind);
            }
            return;
        }
        if (selected < 0)
        {
            socket_error("等待网络就绪");
        }
    }
}

std::int64_t local_port(const std::shared_ptr<state>& value)
{
    const auto native = native_socket(value);
    sockaddr_storage address{};
    int length = sizeof(address);
    if (getsockname(native->get(), reinterpret_cast<sockaddr*>(&address),
            &length) != 0)
    {
        socket_error("查询本地端口");
    }
    if (address.ss_family == AF_INET)
    {
        return ntohs(reinterpret_cast<const sockaddr_in*>(&address)->sin_port);
    }
    if (address.ss_family == AF_INET6)
    {
        return ntohs(reinterpret_cast<const sockaddr_in6*>(&address)->sin6_port);
    }
    network::fail("operation_failed", "本地 socket 地址族无效");
}

[[noreturn]] void socket_error(std::string_view action)
{
    const int code = WSAGetLastError();
    const char* stable = code == WSAETIMEDOUT ? "timeout" :
        code == WSAECONNRESET || code == WSAECONNABORTED ||
        code == WSAENOTCONN || code == WSAESHUTDOWN ? "connection_closed" :
        code == WSAEMSGSIZE ? "size_limit" : "operation_failed";
    network::fail(stable, std::string(action) + "失败，Winsock 错误码 " +
        std::to_string(code));
}

} // namespace tx_generated::socket
