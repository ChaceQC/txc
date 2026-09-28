#include "stdlib/http2_transport.hpp"

#include "stdlib/crypto_internal.hpp"
#include "stdlib/file_stream.hpp"
#include "stdlib/tls_stream.hpp"

#include <mbedtls/net_sockets.h>
#include <mbedtls/platform_util.h>

#include <algorithm>
#include <chrono>
#include <climits>
#include <limits>

namespace tx_generated::http2
{
namespace
{

std::string read_pem(std::string_view path)
{
    stream_file file(path, stream_mode::read);
    std::string result;
    while (true)
    {
        const auto block = file.read(16 * 1024);
        if (block.empty())
        {
            break;
        }
        if (block.size() > 1024 * 1024 - result.size())
        {
            network::fail("size_limit", "TLS PEM 文件超过 1 MiB");
        }
        result += block;
    }
    result.push_back('\0');
    return result;
}

struct private_key_data
{
    explicit private_key_data(std::string_view path) : text(read_pem(path))
    {
    }
    ~private_key_data()
    {
        mbedtls_platform_zeroize(text.data(), text.size());
    }
    std::string text;
};

void require_tls(int result, std::string_view action)
{
    if (result != 0)
    {
        network::fail(result == MBEDTLS_ERR_SSL_TIMEOUT ? "timeout" :
                      "operation_failed", std::string(action) + "失败，TLS 错误码 " +
                      std::to_string(result));
    }
}

} // namespace

tls_config::tls_config(std::string_view cert_path, std::string_view key_path)
{
    if (!crypto::psa_ready())
    {
        network::fail("operation_failed", "初始化 TLS 密码学组件失败");
    }
    mbedtls_entropy_init(&entropy_);
    mbedtls_ctr_drbg_init(&rng_);
    mbedtls_x509_crt_init(&cert_);
    mbedtls_pk_init(&key_);
    mbedtls_ssl_config_init(&config_);
    try
    {
        static constexpr unsigned char personalization[] = "tx-http2-server";
        require_tls(mbedtls_ctr_drbg_seed(&rng_, mbedtls_entropy_func,
            &entropy_, personalization, sizeof(personalization) - 1),
            "初始化 TLS 随机数");
        const auto cert = read_pem(cert_path);
        private_key_data key(key_path);
        require_tls(mbedtls_x509_crt_parse(&cert_,
            reinterpret_cast<const unsigned char*>(cert.data()), cert.size()),
            "解析 TLS 证书");
        require_tls(mbedtls_pk_parse_key(&key_,
            reinterpret_cast<const unsigned char*>(key.text.data()),
            key.text.size(),
            nullptr, 0, mbedtls_ctr_drbg_random, &rng_), "解析 TLS 私钥");
        require_tls(mbedtls_ssl_config_defaults(&config_, MBEDTLS_SSL_IS_SERVER,
            MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT),
            "配置 TLS 服务端");
        mbedtls_ssl_conf_rng(&config_, mbedtls_ctr_drbg_random, &rng_);
        require_tls(mbedtls_ssl_conf_own_cert(&config_, &cert_, &key_),
                    "加载 TLS 证书与私钥");
        static const char* protocols[] = {"h2", nullptr};
        require_tls(mbedtls_ssl_conf_alpn_protocols(&config_, protocols),
                    "配置 HTTP/2 ALPN");
    }
    catch (...)
    {
        release();
        throw;
    }
}

void tls_config::release() noexcept
{
    mbedtls_ssl_config_free(&config_);
    mbedtls_pk_free(&key_);
    mbedtls_x509_crt_free(&cert_);
    mbedtls_ctr_drbg_free(&rng_);
    mbedtls_entropy_free(&entropy_);
}

tls_config::~tls_config() noexcept
{
    release();
}

transport::transport(network::socket_handle socket,
                     const std::shared_ptr<tls_config>& tls,
                     std::int64_t timeout_ms)
    : socket_(std::move(socket)), tls_(tls), timeout_ms_(timeout_ms)
{
    set_receive_timeout(timeout_ms);
    mbedtls_ssl_init(&ssl_);
    try
    {
        if (tls_)
        {
            require_tls(mbedtls_ssl_setup(&ssl_, tls_->get()), "建立 TLS 会话");
            mbedtls_ssl_set_bio(&ssl_, this, tls_send, tls_recv, nullptr);
            int result = 0;
            do
            {
                result = mbedtls_ssl_handshake(&ssl_);
            }
            while (result == MBEDTLS_ERR_SSL_WANT_READ ||
                   result == MBEDTLS_ERR_SSL_WANT_WRITE);
            require_tls(result, "HTTP/2 TLS 握手");
            const auto* protocol = mbedtls_ssl_get_alpn_protocol(&ssl_);
            if (!protocol || std::string_view(protocol) != "h2")
            {
                network::fail("protocol_error", "TLS 对端未协商 HTTP/2 ALPN");
            }
        }
    }
    catch (...)
    {
        mbedtls_ssl_free(&ssl_);
        throw;
    }
}

transport::transport(std::shared_ptr<tls::secure_connection> secure,
                     std::int64_t timeout_ms)
    : secure_(std::move(secure)), timeout_ms_(timeout_ms)
{
    if (timeout_ms_ < 1 || timeout_ms_ > INT_MAX)
    {
        network::fail("invalid_argument", "HTTP/2 TLS 超时无效");
    }
    if (!secure_ || secure_->closed())
    {
        network::fail("connection_closed", "HTTP/2 TLS 安全流已关闭");
    }
    mbedtls_ssl_init(&ssl_);
}

transport::~transport() noexcept
{
    if (secure_)
    {
        mbedtls_ssl_free(&ssl_);
        return;
    }
    if (tls_ && socket_.valid())
    {
        int status = 0;
        do
        {
            status = mbedtls_ssl_close_notify(&ssl_);
        }
        while (status == MBEDTLS_ERR_SSL_WANT_WRITE);
    }
    if (socket_.valid())
    {
        // Windows 在仍有入站数据时直接 closesocket 可能用 RST 丢弃刚发送的帧。
        shutdown(socket_.get(), SD_SEND);
        const auto deadline = std::chrono::steady_clock::now() +
                              std::chrono::seconds(1);
        char buffer[4096];
        while (true)
        {
            const auto remaining = std::chrono::duration_cast<
                std::chrono::milliseconds>(deadline -
                    std::chrono::steady_clock::now()).count();
            if (remaining <= 0)
            {
                break;
            }
            const DWORD drain_timeout = static_cast<DWORD>(remaining);
            setsockopt(socket_.get(), SOL_SOCKET, SO_RCVTIMEO,
                       reinterpret_cast<const char*>(&drain_timeout),
                       sizeof(drain_timeout));
            if (recv(socket_.get(), buffer, sizeof(buffer), 0) <= 0)
            {
                break;
            }
        }
    }
    mbedtls_ssl_free(&ssl_);
}

void transport::close() noexcept
{
    if (secure_)
    {
        try
        {
            secure_->close(1);
        }
        catch (...)
        {
        }
        secure_.reset();
    }
    socket_.reset();
}

void transport::set_receive_timeout(std::int64_t timeout_ms)
{
    timeout_ms_ = timeout_ms;
    if (secure_)
    {
        if (timeout_ms_ < 1 || timeout_ms_ > INT_MAX)
        {
            network::fail("invalid_argument", "HTTP/2 TLS 超时无效");
        }
        return;
    }
    if (timeout_ms < 0 || timeout_ms > std::numeric_limits<DWORD>::max())
    {
        network::fail("invalid_argument", "HTTP/2 读取超时无效");
    }
    const DWORD value = static_cast<DWORD>(timeout_ms);
    if (setsockopt(socket_.get(), SOL_SOCKET, SO_RCVTIMEO,
                   reinterpret_cast<const char*>(&value), sizeof(value)) != 0)
    {
        network::socket_failure("设置 HTTP/2 读取超时");
    }
}

int transport::tls_send(void* context, const unsigned char* data,
                        std::size_t size)
{
    const auto socket = static_cast<transport*>(context)->socket_.get();
    const int sent = send(socket, reinterpret_cast<const char*>(data),
                          static_cast<int>(std::min<std::size_t>(size, INT_MAX)), 0);
    return sent >= 0 ? sent : MBEDTLS_ERR_NET_SEND_FAILED;
}

int transport::tls_recv(void* context, unsigned char* data, std::size_t size)
{
    const auto socket = static_cast<transport*>(context)->socket_.get();
    const int received = recv(socket, reinterpret_cast<char*>(data),
                              static_cast<int>(std::min<std::size_t>(size, INT_MAX)), 0);
    if (received >= 0)
    {
        return received;
    }
    const auto code = WSAGetLastError();
    return code == WSAETIMEDOUT || code == WSAEWOULDBLOCK
        ? MBEDTLS_ERR_SSL_TIMEOUT : MBEDTLS_ERR_NET_RECV_FAILED;
}

std::string transport::read_some(std::size_t max_bytes)
{
    if (secure_)
    {
        auto result = secure_->read(static_cast<std::int64_t>(max_bytes),
                                    timeout_ms_);
        if (result.eof)
        {
            return {};
        }
        return {reinterpret_cast<const char*>(result.data.data()),
                result.data.size()};
    }
    std::string result(max_bytes, '\0');
    int received = 0;
    if (!tls_)
    {
        received = recv(socket_.get(), result.data(),
                        static_cast<int>(result.size()), 0);
        if (received < 0)
        {
            network::socket_failure("读取 HTTP/2 数据");
        }
    }
    else
    {
        do
        {
            received = mbedtls_ssl_read(&ssl_,
                reinterpret_cast<unsigned char*>(result.data()), result.size());
        }
        while (received == MBEDTLS_ERR_SSL_WANT_READ ||
               received == MBEDTLS_ERR_SSL_WANT_WRITE);
        if (received == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
        {
            received = 0;
        }
        if (received < 0)
        {
            require_tls(received, "读取 HTTP/2 TLS 数据");
        }
    }
    result.resize(static_cast<std::size_t>(received));
    return result;
}

void transport::write_all(std::string_view bytes)
{
    while (!bytes.empty())
    {
        if (secure_)
        {
            const auto written = secure_->write(bytes, timeout_ms_);
            if (written <= 0)
            {
                network::fail("connection_closed", "HTTP/2 TLS 安全流已关闭");
            }
            bytes.remove_prefix(static_cast<std::size_t>(written));
            continue;
        }
        int written = 0;
        if (!tls_)
        {
            written = send(socket_.get(), bytes.data(),
                static_cast<int>(std::min<std::size_t>(bytes.size(), INT_MAX)), 0);
            if (written < 0)
            {
                network::socket_failure("发送 HTTP/2 数据");
            }
        }
        else
        {
            written = mbedtls_ssl_write(&ssl_,
                reinterpret_cast<const unsigned char*>(bytes.data()),
                bytes.size());
            if (written == MBEDTLS_ERR_SSL_WANT_READ ||
                written == MBEDTLS_ERR_SSL_WANT_WRITE)
            {
                continue;
            }
            if (written < 0)
            {
                require_tls(written, "发送 HTTP/2 TLS 数据");
            }
        }
        if (written == 0)
        {
            network::fail("connection_closed", "HTTP/2 连接已关闭");
        }
        bytes.remove_prefix(static_cast<std::size_t>(written));
    }
}

} // namespace tx_generated::http2
