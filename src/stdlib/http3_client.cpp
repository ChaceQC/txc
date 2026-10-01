#include "stdlib/http3_client_internal.hpp"

#include "stdlib/encoding.hpp"
#include "stdlib/x509.hpp"

#include <algorithm>
#include <array>
#include <limits>

namespace tx_generated::http3
{
namespace
{

QUIC_BUFFER h3_alpn()
{
    static std::array<std::uint8_t, 2> name{'h', '3'};
    return {static_cast<std::uint32_t>(name.size()), name.data()};
}

std::vector<std::pair<std::string, std::string>> request_fields(
    std::string_view method, const network::parsed_url& address,
    const network::header_map& headers)
{
    std::string authority = detail::wide_to_utf8(address.host);
    if (authority.find(':') != std::string::npos)
    {
        authority = "[" + authority + "]";
    }
    if (address.port != 443)
    {
        authority += ":" + std::to_string(address.port);
    }
    std::vector<std::pair<std::string, std::string>> result{
        {":method", std::string(method)}, {":scheme", "https"},
        {":authority", std::move(authority)},
        {":path", detail::wide_to_utf8(address.target)}
    };
    std::size_t length = 0;
    for (const auto& [name, value] : headers)
    {
        network::validate_header(name, value);
        if (name != network::lower_ascii(name) || name == "host" ||
            name == "content-length" || name == "transfer-encoding" ||
            name == "connection" || name == "upgrade" ||
            name == "proxy-connection" || name == "keep-alive")
        {
            network::fail("invalid_header", "HTTP/3 请求包含保留或大写头字段");
        }
        length += name.size() + value.size();
        if (length > network::max_head_bytes)
        {
            network::fail("size_limit", "HTTP/3 请求头超过 64 KiB");
        }
        result.emplace_back(name, value);
    }
    return result;
}

} // namespace

client_connection::client_connection(std::int64_t timeout_ms, bool binary,
    const std::vector<byte_value>& trust_anchors, const cancel_token* token)
    : library_(quic()), api_(library_.get()),
      trust_anchors_(trust_anchors),
      cancellation_(token ? token->state : nullptr), binary_(binary)
{
    if (timeout_ms < 1 || timeout_ms > 60000)
    {
        network::fail("invalid_argument", "HTTP/3 超时须为 1～60000 毫秒");
    }
    deadline_ = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeout_ms);
    if (token && !cancellation_)
    {
        network::fail("invalid_argument", "HTTP/3 取消令牌无效");
    }
    if (trust_anchors_.size() > 64)
    {
        network::fail("size_limit", "HTTP/3 自定义信任锚超过 64 张");
    }
    for (const auto& anchor : trust_anchors_)
    {
        (void)x509::parse_der(anchor);
    }
    try
    {
        QUIC_SETTINGS settings{};
        settings.IsSet.IdleTimeoutMs = TRUE;
        settings.IdleTimeoutMs = static_cast<std::uint64_t>(timeout_ms);
        settings.IsSet.PeerBidiStreamCount = TRUE;
        settings.PeerBidiStreamCount = 0;
        settings.IsSet.PeerUnidiStreamCount = TRUE;
        settings.PeerUnidiStreamCount = 3;
        settings.IsSet.MigrationEnabled = TRUE;
        settings.MigrationEnabled = TRUE;
        auto alpn = h3_alpn();
        require_quic(api_->ConfigurationOpen(library_.registration(), &alpn,
            1, &settings, sizeof(settings), nullptr, &configuration_),
            "配置 HTTP/3");
        QUIC_CREDENTIAL_CONFIG credential{};
        credential.Type = QUIC_CREDENTIAL_TYPE_NONE;
        credential.Flags = QUIC_CREDENTIAL_FLAG_CLIENT;
        if (!trust_anchors_.empty())
        {
            credential.Flags = static_cast<QUIC_CREDENTIAL_FLAGS>(
                credential.Flags |
                QUIC_CREDENTIAL_FLAG_INDICATE_CERTIFICATE_RECEIVED |
                QUIC_CREDENTIAL_FLAG_DEFER_CERTIFICATE_VALIDATION);
#ifndef _WIN32
            // 通过 DER/PKCS#7 交换证书，避免依赖 MsQuic 内部 OpenSSL 的对象 ABI。
            credential.Flags = static_cast<QUIC_CREDENTIAL_FLAGS>(credential.Flags |
                QUIC_CREDENTIAL_FLAG_USE_PORTABLE_CERTIFICATES);
#endif
        }
        require_quic(api_->ConfigurationLoadCredential(configuration_,
            &credential), "配置 HTTP/3 证书验证");
        nghttp3_callbacks callbacks{};
        callbacks.begin_headers = on_begin_headers;
        callbacks.recv_header = on_header;
        callbacks.recv_data = on_data;
        callbacks.end_stream = on_end_stream;
        nghttp3_settings h3_settings;
        nghttp3_settings_default(&h3_settings);
        h3_settings.max_field_section_size = network::max_head_bytes;
        h3_settings.qpack_max_dtable_capacity = 0;
        if (nghttp3_conn_client_new(&h3_, &callbacks, &h3_settings,
                                     nullptr, this) != 0)
        {
            network::fail("operation_failed", "初始化 HTTP/3 帧处理失败");
        }
        require_quic(api_->ConnectionOpen(library_.registration(),
            on_connection, this, &connection_), "创建 QUIC 连接");
    }
    catch (...)
    {
        if (connection_)
        {
            api_->ConnectionClose(connection_);
        }
        if (h3_)
        {
            nghttp3_conn_del(h3_);
        }
        if (configuration_)
        {
            api_->ConfigurationClose(configuration_);
        }
        throw;
    }
}

client_connection::~client_connection() noexcept
{
    if (validation_thread_.joinable())
    {
        validation_thread_.join();
    }
    if (connection_)
    {
        bool complete = false;
        {
            std::lock_guard lock(mutex_);
            complete = response_done_;
        }
        const auto flags = complete
            ? QUIC_CONNECTION_SHUTDOWN_FLAG_NONE
            : QUIC_CONNECTION_SHUTDOWN_FLAG_SILENT;
        api_->ConnectionShutdown(connection_, flags, 0);
        {
            std::unique_lock lock(mutex_);
            changed_.wait_for(lock, std::chrono::milliseconds(100), [&]
            {
                return shutdown_;
            });
        }
        for (const auto& stream : streams_)
        {
            if (stream->handle)
            {
                api_->StreamClose(stream->handle);
            }
        }
        api_->ConnectionClose(connection_);
    }
    if (h3_)
    {
        nghttp3_conn_del(h3_);
    }
    if (configuration_)
    {
        api_->ConfigurationClose(configuration_);
    }
}

http_response_data client_connection::send(std::string_view method,
    std::string_view url, const network::header_map& headers,
    std::string_view body)
{
    network::validate_token(method, "HTTP 方法");
    if (body.size() > network::max_body_bytes)
    {
        network::fail("size_limit", "HTTP/3 请求正文超过 8 MiB");
    }
    if (!binary_)
    {
        network::validate_utf8(body);
    }
    const auto address = network::parse_url(url, false);
    if (!address.secure)
    {
        network::fail("invalid_url", "HTTP/3 只接受 HTTPS URL");
    }
    body_ = body;
    method_ = method;
    check_cancellation();
    hostname_ = detail::wide_to_utf8(address.host);
    require_quic(api_->ConnectionStart(connection_, configuration_,
        AF_UNSPEC, hostname_.c_str(), address.port), "开始 QUIC 握手");
    wait_connected();
    open_http3_streams();
    submit_request(method, address, headers);
    await_response();
    if (!binary_)
    {
        network::validate_utf8(response_.body);
    }
    response_.body_length = static_cast<std::int64_t>(response_.body.size());
    response_.protocol = "h3";
    return std::move(response_);
}

void client_connection::wait_connected()
{
    std::unique_lock lock(mutex_);
    while (!connected_ && !shutdown_ && !error_)
    {
        check_cancellation();
        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline_)
        {
            network::fail("timeout", "HTTP/3 TLS 握手超时");
        }
        changed_.wait_until(lock, std::min(deadline_, now +
            std::chrono::milliseconds(20)));
    }
    check_cancellation();
    check_error();
    if (!connected_)
    {
        network::fail("connection_closed", "HTTP/3 TLS 握手未完成");
    }
}

client_connection::stream_state& client_connection::open_stream(
    QUIC_STREAM_OPEN_FLAGS flags)
{
    auto state = std::make_unique<stream_state>();
    state->owner = this;
    require_quic(api_->StreamOpen(connection_, flags, on_stream,
        state.get(), &state->handle), "打开 QUIC 流");
    auto& result = *state;
    streams_.push_back(std::move(state));
    require_quic(api_->StreamStart(result.handle,
        QUIC_STREAM_START_FLAG_IMMEDIATE), "启动 QUIC 流");
    std::unique_lock lock(mutex_);
    while (!result.started && !shutdown_ && !error_)
    {
        check_cancellation();
        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline_)
        {
            network::fail("timeout", "等待 QUIC 流启动超时");
        }
        changed_.wait_until(lock, std::min(deadline_, now +
            std::chrono::milliseconds(20)));
    }
    check_cancellation();
    check_error();
    if (!result.started)
    {
        network::fail("connection_closed", "QUIC 流未能启动");
    }
    return result;
}

void client_connection::open_http3_streams()
{
    const auto control = open_stream(QUIC_STREAM_OPEN_FLAG_UNIDIRECTIONAL).id;
    const auto encoder = open_stream(QUIC_STREAM_OPEN_FLAG_UNIDIRECTIONAL).id;
    const auto decoder = open_stream(QUIC_STREAM_OPEN_FLAG_UNIDIRECTIONAL).id;
    {
        std::lock_guard lock(mutex_);
        if (nghttp3_conn_bind_control_stream(h3_, control) != 0 ||
            nghttp3_conn_bind_qpack_streams(h3_, encoder, decoder) != 0)
        {
            network::fail("protocol_error", "绑定 HTTP/3 控制流失败");
        }
        pump();
    }
    response_stream_ = open_stream(QUIC_STREAM_OPEN_FLAG_NONE).id;
}

void client_connection::submit_request(std::string_view method,
    const network::parsed_url& address,
    const network::header_map& headers)
{
    auto fields = request_fields(method, address, headers);
    std::vector<nghttp3_nv> values;
    values.reserve(fields.size());
    for (const auto& [name, value] : fields)
    {
        values.push_back({reinterpret_cast<const std::uint8_t*>(name.data()),
            reinterpret_cast<const std::uint8_t*>(value.data()),
            name.size(), value.size(), NGHTTP3_NV_FLAG_NONE});
    }
    nghttp3_data_reader reader{read_body};
    std::lock_guard lock(mutex_);
    if (nghttp3_conn_submit_request(h3_, response_stream_, values.data(),
            values.size(), body_.empty() ? nullptr : &reader, nullptr) != 0)
    {
        network::fail("protocol_error", "提交 HTTP/3 请求失败");
    }
    pump();
}

void client_connection::await_response()
{
    std::unique_lock lock(mutex_);
    while (!response_done_ && !shutdown_ && !error_)
    {
        check_cancellation();
        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline_)
        {
            network::fail("timeout", "等待 HTTP/3 响应超时");
        }
        changed_.wait_until(lock, std::min(deadline_, now +
            std::chrono::milliseconds(20)));
    }
    check_cancellation();
    check_error();
    if (!response_done_ || response_.status < 200)
    {
        network::fail("protocol_error", "HTTP/3 响应未完整结束");
    }
}

http_response_data client_send(std::string_view method,
    std::string_view url, const network::header_map& headers,
    std::string_view body, std::int64_t timeout_ms, bool binary,
    const std::vector<byte_value>& trust_anchors, const cancel_token* token)
{
    client_connection connection(timeout_ms, binary, trust_anchors, token);
    return connection.send(method, url, headers, body);
}

} // namespace tx_generated::http3
