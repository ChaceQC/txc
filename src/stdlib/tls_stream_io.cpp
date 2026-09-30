#include "stdlib/tls_stream.hpp"

#include "stdlib/error.hpp"

#include <mbedtls/net_sockets.h>
#include <algorithm>
#include <chrono>
#include <climits>
#include <exception>
#include <string>
#include <vector>

namespace tx_generated::tls
{
namespace
{

constexpr std::size_t transfer_chunk = 16 * 1024;
constexpr std::size_t maximum_transfer = 16 * 1024 * 1024;

void check_tls_timeout(std::int64_t timeout_ms)
{
    if (timeout_ms < 1 || timeout_ms > INT_MAX)
    {
        network::fail("invalid_argument", "TLS 超时须为 1～2147483647 毫秒");
    }
}

[[noreturn]] void transfer_error(std::string_view action, int status)
{
    throw runtime_failure({tx::error_kind::security, "operation_failed",
        std::string(action) + "失败，TLS 错误码 " + std::to_string(status)});
}

} // namespace

secure_connection::~secure_connection() noexcept
{
    release();
}

void secure_connection::release() noexcept
{
    closed_ = true;
    if (released_)
    {
        return;
    }
    released_ = true;
    mbedtls_ssl_free(&ssl_);
    mbedtls_ssl_config_free(&config_);
    release_identity_material(identity_owner_, std::move(identity_material_));
    identity_owner_.reset();
    native_.reset();
}

bool secure_connection::closed() const noexcept
{
    return closed_;
}

std::string secure_connection::negotiated_alpn() const
{
    std::lock_guard lock(mutex_);
    if (closed_)
    {
        network::fail("connection_closed", "TLS 安全流已关闭");
    }
    return selected_alpn_;
}

int secure_connection::tls_send(void* context, const unsigned char* data,
                                std::size_t size)
{
    const auto* self = static_cast<secure_connection*>(context);
    const int sent = network::socket_send(self->native_->get(),
        reinterpret_cast<const char*>(data),
        static_cast<int>(std::min<std::size_t>(size, INT_MAX)), 0);
    if (sent >= 0)
    {
        return sent;
    }
    return WSAGetLastError() == WSAEWOULDBLOCK
        ? MBEDTLS_ERR_SSL_WANT_WRITE : MBEDTLS_ERR_NET_SEND_FAILED;
}

int secure_connection::tls_recv(void* context, unsigned char* data,
                                std::size_t size)
{
    const auto* self = static_cast<secure_connection*>(context);
    const int received = network::socket_receive(self->native_->get(),
        reinterpret_cast<char*>(data),
        static_cast<int>(std::min<std::size_t>(size, INT_MAX)), 0);
    if (received >= 0)
    {
        return received;
    }
    return WSAGetLastError() == WSAEWOULDBLOCK
        ? MBEDTLS_ERR_SSL_WANT_READ : MBEDTLS_ERR_NET_RECV_FAILED;
}

void secure_connection::wait_io(bool write,
    std::chrono::steady_clock::time_point deadline)
{
    while (true)
    {
        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline)
        {
            network::fail("timeout", "TLS 网络操作超时");
        }
        const auto remaining = std::chrono::duration_cast<
            std::chrono::milliseconds>(deadline - now).count();
        const auto slice = std::min<std::int64_t>(remaining + 1, 10);
        const int selected = network::wait_socket(native_->get(), write,
            static_cast<int>(slice));
        if (selected > 0)
        {
            return;
        }
        if (selected < 0)
        {
            network::socket_failure("等待 TLS 网络就绪");
        }
    }
}

socket::read_result secure_connection::read(std::int64_t max_bytes,
                                            std::int64_t timeout_ms)
{
    check_tls_timeout(timeout_ms);
    if (max_bytes < 1 || max_bytes > static_cast<std::int64_t>(maximum_transfer))
    {
        network::fail("invalid_argument", "TLS 读取长度必须为 1～16 MiB");
    }
    std::lock_guard lock(mutex_);
    if (closed_)
    {
        network::fail("connection_closed", "TLS 安全流已关闭");
    }
    if (peer_eof_)
    {
        return {{}, true};
    }
    socket::read_result result;
    result.data.resize(std::min<std::size_t>(max_bytes, transfer_chunk));
    const auto deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeout_ms);
    while (true)
    {
        const int received = mbedtls_ssl_read(&ssl_, result.data.data(),
            result.data.size());
        if (received > 0)
        {
            result.data.resize(static_cast<std::size_t>(received));
            return result;
        }
        if (received == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
        {
            peer_eof_ = true;
            return {{}, true};
        }
        if (received == MBEDTLS_ERR_SSL_WANT_READ ||
            received == MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            wait_io(received == MBEDTLS_ERR_SSL_WANT_WRITE, deadline);
            continue;
        }
#ifdef MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET
        if (received == MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET)
        {
            continue;
        }
#endif
        release();
        if (received == 0 || received == MBEDTLS_ERR_SSL_CONN_EOF)
        {
            throw runtime_failure({tx::error_kind::security,
                "truncated_close", "TLS 对端未发送 close_notify 就关闭了连接"});
        }
        transfer_error("读取 TLS 应用数据", received);
    }
}

std::int64_t secure_connection::write(std::string_view data,
                                      std::int64_t timeout_ms)
{
    check_tls_timeout(timeout_ms);
    if (data.size() > maximum_transfer)
    {
        network::fail("size_limit", "TLS 单次写入输入超过 16 MiB");
    }
    std::lock_guard lock(mutex_);
    if (closed_)
    {
        network::fail("connection_closed", "TLS 安全流已关闭");
    }
    if (data.empty())
    {
        return 0;
    }
    const auto deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeout_ms);
    while (true)
    {
        const int sent = mbedtls_ssl_write(&ssl_,
            reinterpret_cast<const unsigned char*>(data.data()),
            std::min(data.size(), transfer_chunk));
        if (sent > 0)
        {
            return sent;
        }
        if (sent == MBEDTLS_ERR_SSL_WANT_READ ||
            sent == MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            wait_io(sent == MBEDTLS_ERR_SSL_WANT_WRITE, deadline);
            continue;
        }
        release();
        if (sent == 0)
        {
            network::fail("connection_closed", "TLS 对端已关闭连接");
        }
        transfer_error("写入 TLS 应用数据", sent);
    }
}

void secure_connection::close(std::int64_t timeout_ms)
{
    std::lock_guard lock(mutex_);
    if (closed_)
    {
        return;
    }
    try
    {
        check_tls_timeout(timeout_ms);
        const auto deadline = std::chrono::steady_clock::now() +
            std::chrono::milliseconds(timeout_ms);
        while (true)
        {
            const int status = mbedtls_ssl_close_notify(&ssl_);
            if (status == 0)
            {
                break;
            }
            if (status == MBEDTLS_ERR_SSL_WANT_READ ||
                status == MBEDTLS_ERR_SSL_WANT_WRITE)
            {
                wait_io(status == MBEDTLS_ERR_SSL_WANT_WRITE, deadline);
                continue;
            }
            network::fail("operation_failed", "发送 TLS close_notify 失败");
        }
    }
    catch (...)
    {
        release();
        throw;
    }
    release();
}

} // namespace tx_generated::tls
