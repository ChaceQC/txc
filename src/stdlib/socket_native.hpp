#pragma once

#include <chrono>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <cerrno>
#include <cstdint>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

// 现有 socket 领域接口共用这些类型；仅在此处映射操作系统差异。
using SOCKET = int;
using DWORD = std::uint32_t;
using ULONG = unsigned long;
using WSAPOLLFD = pollfd;
inline constexpr int INVALID_SOCKET = -1;
inline constexpr int SOCKET_ERROR = -1;
inline constexpr int WSAEWOULDBLOCK = EAGAIN;
inline constexpr int WSAEINPROGRESS = EINPROGRESS;
inline constexpr int WSAETIMEDOUT = ETIMEDOUT;
inline constexpr int WSAECONNRESET = ECONNRESET;
inline constexpr int WSAECONNABORTED = ECONNABORTED;
inline constexpr int WSAENOTCONN = ENOTCONN;
inline constexpr int WSAESHUTDOWN = EPIPE;
inline constexpr int WSAEMSGSIZE = EMSGSIZE;
inline constexpr int WSAEINTR = EINTR;
inline constexpr int SD_RECEIVE = SHUT_RD;
inline constexpr int SD_SEND = SHUT_WR;
inline constexpr int SD_BOTH = SHUT_RDWR;

inline int WSAGetLastError()
{
    // 非阻塞 connect 的 EINPROGRESS 与 Windows 的 WOULD_BLOCK 对应。
    return errno == EINPROGRESS ? EAGAIN : errno;
}
inline void WSASetLastError(int error)
{
    errno = error;
}
inline int closesocket(int socket)
{
    return close(socket);
}
inline int ioctlsocket(int socket, unsigned long request, unsigned long* value)
{
    int flag = static_cast<int>(*value);
    return ioctl(socket, request, &flag);
}
inline int WSAPoll(pollfd* descriptors, unsigned long count, int timeout)
{
    return poll(descriptors, count, timeout);
}
#endif

namespace tx_generated::network
{

#ifdef _WIN32
using socket_length = int;
#else
using socket_length = socklen_t;
#endif

inline SOCKET create_native_socket(int family, int type, int protocol)
{
#ifdef _WIN32
    return WSASocketW(family, type, protocol, nullptr, 0, WSA_FLAG_OVERLAPPED);
#else
    return ::socket(family, type | SOCK_CLOEXEC, protocol);
#endif
}

inline SOCKET accept_native_socket(SOCKET socket)
{
#ifdef _WIN32
    return ::accept(socket, nullptr, nullptr);
#else
    return ::accept4(socket, nullptr, nullptr, SOCK_CLOEXEC);
#endif
}

inline int wait_socket(SOCKET socket, bool write, int timeout_ms)
{
    const auto deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeout_ms);
    while (true)
    {
        WSAPOLLFD descriptor{socket, static_cast<short>(write ? POLLWRNORM : POLLRDNORM), 0};
        const auto status = WSAPoll(&descriptor, 1, timeout_ms);
        if (status >= 0 || WSAGetLastError() != WSAEINTR)
        {
            return status;
        }
        timeout_ms = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now()).count());
        if (timeout_ms <= 0)
        {
            return 0;
        }
    }
}

inline int socket_receive(SOCKET socket, char* data, int size, int flags)
{
#ifdef _WIN32
    return ::recv(socket, data, size, flags);
#else
    ssize_t received;
    do
    {
        received = ::recv(socket, data, size, flags);
    } while (received < 0 && errno == EINTR);
    return static_cast<int>(received);
#endif
}

inline int set_socket_exclusive(SOCKET socket)
{
#ifdef _WIN32
    const BOOL exclusive = TRUE;
    return ::setsockopt(socket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
        reinterpret_cast<const char*>(&exclusive), sizeof(exclusive));
#else
    // Linux 默认禁止重复绑定；明确禁用复用以保留独占监听语义。
    const int disabled = 0;
    return ::setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &disabled, sizeof(disabled));
#endif
}

inline int socket_send(SOCKET socket, const char* data, int size, int flags)
{
#ifdef _WIN32
    return ::send(socket, data, size, flags);
#else
    // 对端关闭只产生可捕获的 I/O 错误，不向调用程序发送 SIGPIPE。
    ssize_t sent;
    do
    {
        sent = ::send(socket, data, size, flags | MSG_NOSIGNAL);
    } while (sent < 0 && errno == EINTR);
    return static_cast<int>(sent);
#endif
}

inline int socket_select(int count, fd_set* readers, fd_set* writers,
    fd_set* errors, timeval* timeout)
{
#ifndef _WIN32
    count = 0;
    for (int descriptor = 0; descriptor < FD_SETSIZE; ++descriptor)
    {
        if ((readers && FD_ISSET(descriptor, readers)) ||
            (writers && FD_ISSET(descriptor, writers)) ||
            (errors && FD_ISSET(descriptor, errors)))
        {
            count = descriptor + 1;
        }
    }
#endif
    return ::select(count, readers, writers, errors, timeout);
}

inline int set_socket_option(SOCKET socket, int level, int option,
    const char* value, int size)
{
#ifndef _WIN32
    if (level == SOL_SOCKET && (option == SO_RCVTIMEO || option == SO_SNDTIMEO))
    {
        const auto milliseconds = *reinterpret_cast<const DWORD*>(value);
        timeval duration{static_cast<time_t>(milliseconds / 1000),
            static_cast<suseconds_t>((milliseconds % 1000) * 1000)};
        return ::setsockopt(socket, level, option, &duration, sizeof(duration));
    }
#endif
    return ::setsockopt(socket, level, option, value, size);
}

inline int get_socket_option(SOCKET socket, int level, int option, char* value, int* size)
{
#ifdef _WIN32
    return ::getsockopt(socket, level, option, value, size);
#else
    auto length = static_cast<socklen_t>(*size);
    const auto result = ::getsockopt(socket, level, option, value, &length);
    *size = static_cast<int>(length);
    return result;
#endif
}

} // namespace tx_generated::network
