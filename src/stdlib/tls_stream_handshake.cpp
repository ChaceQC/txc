#include "stdlib/tls_stream.hpp"

#include "stdlib/bytes.hpp"
#include "stdlib/crypto_internal.hpp"
#include "stdlib/error.hpp"
#include "stdlib/x509.hpp"

#include <mbedtls/ssl.h>

#include <algorithm>
#include <chrono>
#include <climits>
#include <string>
#include <unordered_set>
#include <utility>

namespace tx_generated::tls
{
namespace
{

[[noreturn]] void security_error(std::string code, std::string message)
{
    throw runtime_failure({tx::error_kind::security,
        std::move(code), std::move(message)});
}

void require_success(int status, std::string_view action)
{
    if (status != 0)
    {
        security_error("handshake_failed", std::string(action) +
            "失败，TLS 错误码 " + std::to_string(status));
    }
}

byte_value certificate_bytes(const mbedtls_x509_crt* certificate)
{
    return make_bytes(std::vector<std::uint8_t>(certificate->raw.p,
        certificate->raw.p + certificate->raw.len));
}

void check_tls_timeout(std::int64_t timeout_ms)
{
    if (timeout_ms < 1 || timeout_ms > INT_MAX)
    {
        network::fail("invalid_argument", "TLS 超时须为 1～2147483647 毫秒");
    }
}

} // namespace

void validate_alpn(const std::vector<std::string>& protocols,
                   std::int64_t timeout_ms)
{
    check_tls_timeout(timeout_ms);
    if (protocols.size() > 16)
    {
        security_error("size_limit", "TLS ALPN 协议数量超过 16");
    }
    std::unordered_set<std::string> seen;
    for (const auto& protocol : protocols)
    {
        if (protocol.empty() || protocol.size() > 255 ||
            !std::all_of(protocol.begin(), protocol.end(), [](unsigned char byte)
            {
                return byte >= 0x21 && byte <= 0x7e;
            }) || !seen.insert(protocol).second)
        {
            security_error("invalid_argument", "TLS ALPN 协议名称无效或重复");
        }
    }
}

secure_connection::secure_connection(
    std::shared_ptr<network::socket_handle> native,
    client_options options, std::vector<std::string> protocols,
    std::int64_t timeout_ms)
    : native_(std::move(native)), allow_no_alpn_(options.allow_no_alpn),
      identity_id_(options.identity_id),
      hostname_(std::move(options.hostname)),
      trust_(std::move(options.trust)), protocols_(std::move(protocols))
{
    initialize(timeout_ms);
}

secure_connection::secure_connection(
    std::shared_ptr<network::socket_handle> native,
    server_options options, std::vector<std::string> protocols,
    std::int64_t timeout_ms)
    : native_(std::move(native)), server_(true),
      require_client_identity_(options.require_client_identity),
      identity_id_(options.identity_id), trust_(std::move(options.trust)),
      protocols_(std::move(protocols))
{
    initialize(timeout_ms);
}

void secure_connection::load_identity()
{
    if (identity_id_ == 0)
    {
        if (server_)
        {
            security_error("invalid_state", "TLS 服务端缺少证书身份");
        }
        return;
    }
    identity_owner_ = get_identity(identity_id_);
    identity_material_ = acquire_identity_material(identity_owner_);
    require_success(mbedtls_ssl_conf_own_cert(&config_, &identity_material_->certificates, &identity_material_->key),
        "配置 TLS 身份");
}

void secure_connection::initialize(std::int64_t timeout_ms)
{
    validate_alpn(protocols_, timeout_ms);
    if (!native_ || !native_->valid() || !crypto::psa_ready())
    {
        security_error("operation_failed", "TLS 连接或密码学组件不可用");
    }
    // 握手包含多个短记录；关闭 Nagle，避免等待对端延迟 ACK 才发送下一记录。
    const int no_delay = 1;
    if (setsockopt(native_->get(), IPPROTO_TCP, TCP_NODELAY,
            reinterpret_cast<const char*>(&no_delay), sizeof(no_delay)) != 0)
    {
        network::fail("operation_failed", "配置 TLS TCP_NODELAY 失败");
    }
    mbedtls_ssl_config_init(&config_);
    mbedtls_ssl_init(&ssl_);
    try
    {
        require_success(mbedtls_ssl_config_defaults(&config_,
            server_ ? MBEDTLS_SSL_IS_SERVER : MBEDTLS_SSL_IS_CLIENT,
            MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT),
            "初始化 TLS 配置");
        mbedtls_ssl_conf_min_tls_version(&config_, MBEDTLS_SSL_VERSION_TLS1_2);
        mbedtls_ssl_conf_rng(&config_, tls_random, nullptr);
        // 链验证统一交由现有 Windows X.509 信任引擎；握手时请求证书，
        // 但绝不在应用层验证完成前返回安全流或交付明文。
        mbedtls_ssl_conf_authmode(&config_, MBEDTLS_SSL_VERIFY_OPTIONAL);
        load_identity();
        if (!protocols_.empty())
        {
            protocol_pointers_.reserve(protocols_.size() + 1);
            for (const auto& protocol : protocols_)
            {
                protocol_pointers_.push_back(protocol.c_str());
            }
            protocol_pointers_.push_back(nullptr);
            require_success(mbedtls_ssl_conf_alpn_protocols(&config_,
                protocol_pointers_.data()), "配置 TLS ALPN");
        }
        require_success(mbedtls_ssl_setup(&ssl_, &config_), "建立 TLS 会话");
        if (!server_)
        {
            require_success(mbedtls_ssl_set_hostname(&ssl_, hostname_.c_str()),
                "设置 TLS SNI 与主机名");
        }
        mbedtls_ssl_set_bio(&ssl_, this, tls_send, tls_recv, nullptr);
        handshake(timeout_ms);
        verify_peer();
        if (const auto* selected = mbedtls_ssl_get_alpn_protocol(&ssl_))
        {
            selected_alpn_ = selected;
        }
        if (!protocols_.empty() && selected_alpn_.empty() && !allow_no_alpn_)
        {
            security_error("alpn_mismatch", "TLS 对端没有协商所需 ALPN 协议");
        }
    }
    catch (...)
    {
        release();
        throw;
    }
}

void secure_connection::handshake(std::int64_t timeout_ms)
{
    const auto deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeout_ms);
    while (true)
    {
        const int status = mbedtls_ssl_handshake(&ssl_);
        if (status == 0)
        {
            return;
        }
        if (status == MBEDTLS_ERR_SSL_WANT_READ ||
            status == MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            wait_io(status == MBEDTLS_ERR_SSL_WANT_WRITE, deadline);
            continue;
        }
        if (status == MBEDTLS_ERR_SSL_NO_APPLICATION_PROTOCOL)
        {
            security_error("alpn_mismatch", "TLS 对端没有共同的 ALPN 协议");
        }
        security_error("handshake_failed", "TLS 握手失败，错误码 " +
            std::to_string(status));
    }
}

void secure_connection::verify_peer()
{
    const auto* peer = mbedtls_ssl_get_peer_cert(&ssl_);
    if (!peer)
    {
        if (server_ && !require_client_identity_)
        {
            return;
        }
        security_error("certificate_required", "TLS 对端没有提供证书");
    }
    auto leaf = certificate_bytes(peer);
    bytes_vector intermediates;
    for (peer = peer->next; peer; peer = peer->next)
    {
        if (intermediates.data().values.size() >= 64)
        {
            security_error("size_limit", "TLS 对端证书链超过 64 张");
        }
        intermediates.data().values.push_back(certificate_bytes(peer));
    }
    intermediates.data().refresh();
    const auto verification = x509::verify(leaf, intermediates,
        trust_.anchors, server_ ? "" : hostname_,
        server_ ? "client_auth" : "server_auth", trust_.system_roots);
    if (verification.status != "valid")
    {
        security_error(verification.status,
            "TLS 对端证书验证失败：" + verification.status);
    }
}

} // namespace tx_generated::tls
