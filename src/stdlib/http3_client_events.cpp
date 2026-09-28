#include "stdlib/http3_client_internal.hpp"

#include "stdlib/error.hpp"
#include "stdlib/x509.hpp"

#include <algorithm>
#include <cstring>
#include <limits>

namespace tx_generated::http3
{
namespace
{

bool certificate_failure(QUIC_STATUS status)
{
    return status == QUIC_STATUS_BAD_CERTIFICATE ||
        status == QUIC_STATUS_UNSUPPORTED_CERTIFICATE ||
        status == QUIC_STATUS_REVOKED_CERTIFICATE ||
        status == QUIC_STATUS_EXPIRED_CERTIFICATE ||
        status == QUIC_STATUS_UNKNOWN_CERTIFICATE ||
        status == QUIC_STATUS_TLS_ALERT(QUIC_TLS_ALERT_CODE_UNKNOWN_CA) ||
        status == QUIC_STATUS_REQUIRED_CERTIFICATE ||
        status == QUIC_STATUS_CERT_EXPIRED ||
        status == QUIC_STATUS_CERT_UNTRUSTED_ROOT ||
        status == QUIC_STATUS_CERT_NO_CERT;
}

} // namespace

void client_connection::capture_error() noexcept
{
    std::lock_guard lock(mutex_);
    if (!error_)
    {
        error_ = std::current_exception();
    }
    changed_.notify_all();
}

void client_connection::check_error()
{
    if (error_)
    {
        std::rethrow_exception(error_);
    }
}

void client_connection::check_cancellation()
{
    if (!cancellation_)
    {
        return;
    }
    std::lock_guard lock(cancellation_->mutex);
    if (cancellation_->cancelled)
    {
        throw runtime_failure({tx::error_kind::cancelled, "cancelled",
            "HTTP/3 请求已取消"});
    }
    if (cancellation_->deadline &&
        std::chrono::steady_clock::now() >= *cancellation_->deadline)
    {
        throw runtime_failure({tx::error_kind::cancelled,
            "deadline_exceeded", "HTTP/3 请求截止时间已到"});
    }
}

QUIC_STATUS QUIC_API client_connection::on_connection(HQUIC, void* context,
                                                        QUIC_CONNECTION_EVENT* event)
{
    auto* self = static_cast<client_connection*>(context);
    try
    {
        if (event->Type == QUIC_CONNECTION_EVENT_PEER_CERTIFICATE_RECEIVED)
        {
            self->validate_peer(event);
            return QUIC_STATUS_PENDING;
        }
        self->connection_event(event);
        return QUIC_STATUS_SUCCESS;
    }
    catch (...)
    {
        self->capture_error();
        return QUIC_STATUS_INTERNAL_ERROR;
    }
}

void client_connection::validate_peer(QUIC_CONNECTION_EVENT* event)
{
    const auto* certificate = static_cast<PCCERT_CONTEXT>(
        event->PEER_CERTIFICATE_RECEIVED.Certificate);
    if (!certificate || validation_thread_.joinable())
    {
        network::fail("security_error", "HTTP/3 服务端证书缺失或重复");
    }
    auto copy_der = [](PCCERT_CONTEXT source) -> byte_value
    {
        return std::make_shared<const std::vector<std::uint8_t>>(
            source->pbCertEncoded,
            source->pbCertEncoded + source->cbCertEncoded);
    };
    auto leaf = copy_der(certificate);
    std::vector<byte_value> intermediates;
    const auto* chain = static_cast<const CERT_CHAIN_CONTEXT*>(
        event->PEER_CERTIFICATE_RECEIVED.Chain);
    if (chain)
    {
        for (DWORD index = 0; index < chain->cChain; ++index)
        {
            const auto* part = chain->rgpChain[index];
            for (DWORD item = 0; item < part->cElement; ++item)
            {
                const auto* current = part->rgpElement[item]->pCertContext;
                if (current != certificate && intermediates.size() < 64)
                {
                    intermediates.push_back(copy_der(current));
                }
            }
        }
    }
    validation_thread_ = std::thread([this, leaf,
        intermediates = std::move(intermediates)]
    {
        bool accepted = false;
        try
        {
            bytes_vector chain_values;
            chain_values.data().values = intermediates;
            chain_values.data().refresh();
            bytes_vector roots;
            roots.data().values = trust_anchors_;
            roots.data().refresh();
            const auto result = x509::verify(leaf, chain_values, roots,
                hostname_, "server_auth", false);
            accepted = result.status == "valid";
            if (!accepted)
            {
                throw runtime_failure({tx::error_kind::security,
                    "certificate_invalid", "HTTP/3 服务端证书验证失败：" +
                    result.status});
            }
        }
        catch (...)
        {
            capture_error();
        }
        const auto status = api_->ConnectionCertificateValidationComplete(
            connection_, accepted,
            accepted ? QUIC_TLS_ALERT_CODE_SUCCESS :
                       QUIC_TLS_ALERT_CODE_BAD_CERTIFICATE);
        if (QUIC_FAILED(status))
        {
            try
            {
                network::fail("security_error", "提交 HTTP/3 证书验证结果失败");
            }
            catch (...)
            {
                capture_error();
            }
        }
    });
}

void client_connection::connection_event(QUIC_CONNECTION_EVENT* event)
{
    if (event->Type == QUIC_CONNECTION_EVENT_CONNECTED)
    {
        std::lock_guard lock(mutex_);
        connected_ = true;
        changed_.notify_all();
    }
    else if (event->Type == QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT)
    {
        const auto status = event->SHUTDOWN_INITIATED_BY_TRANSPORT.Status;
        std::lock_guard lock(mutex_);
        if (!error_ && !response_done_)
        {
            try
            {
                const auto code = certificate_failure(status)
                    ? "security_error" :
                    status == QUIC_STATUS_ALPN_NEG_FAILURE ||
                    status == QUIC_STATUS_CONNECTION_REFUSED ||
                    status == QUIC_STATUS_UNREACHABLE
                    ? "unsupported_protocol" : "operation_failed";
                network::fail(code, "QUIC 握手或传输失败，状态码 " +
                    std::to_string(status));
            }
            catch (...)
            {
                error_ = std::current_exception();
            }
        }
        changed_.notify_all();
    }
    else if (event->Type == QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE)
    {
        std::lock_guard lock(mutex_);
        shutdown_ = true;
        changed_.notify_all();
    }
    else if (event->Type == QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED)
    {
        auto state = std::make_unique<stream_state>();
        state->owner = this;
        state->handle = event->PEER_STREAM_STARTED.Stream;
        std::uint32_t length = sizeof(state->id);
        require_quic(api_->GetParam(state->handle, QUIC_PARAM_STREAM_ID,
            &length, &state->id), "读取 HTTP/3 对端流编号");
        state->started = true;
        auto* raw = state.get();
        {
            std::lock_guard lock(mutex_);
            stream_handles_.emplace(state->id, state->handle);
            streams_.push_back(std::move(state));
        }
        api_->SetCallbackHandler(raw->handle,
            reinterpret_cast<void*>(on_stream), raw);
    }
}

QUIC_STATUS QUIC_API client_connection::on_stream(HQUIC, void* context,
                                                    QUIC_STREAM_EVENT* event)
{
    auto* state = static_cast<stream_state*>(context);
    try
    {
        state->owner->stream_event(*state, event);
        return QUIC_STATUS_SUCCESS;
    }
    catch (...)
    {
        state->owner->capture_error();
        return QUIC_STATUS_INTERNAL_ERROR;
    }
}

void client_connection::receive(stream_state& stream,
    const QUIC_BUFFER* buffers, std::uint32_t count, bool fin)
{
    if (!h3_)
    {
        network::fail("protocol_error", "HTTP/3 会话尚未初始化");
    }
    for (std::uint32_t index = 0; index < count; ++index)
    {
        const auto status = nghttp3_conn_read_stream(h3_, stream.id,
            buffers[index].Buffer, buffers[index].Length, 0);
        if (status < 0)
        {
            network::fail("protocol_error", "HTTP/3 接收帧无效");
        }
    }
    if (fin && nghttp3_conn_read_stream(h3_, stream.id, nullptr, 0, 1) < 0)
    {
        network::fail("protocol_error", "HTTP/3 流结束帧无效");
    }
    pump();
}

void client_connection::stream_event(stream_state& stream,
                                      QUIC_STREAM_EVENT* event)
{
    if (event->Type == QUIC_STREAM_EVENT_START_COMPLETE)
    {
        std::lock_guard lock(mutex_);
        require_quic(event->START_COMPLETE.Status, "启动 HTTP/3 流");
        stream.id = static_cast<std::int64_t>(event->START_COMPLETE.ID);
        stream.started = true;
        stream_handles_[stream.id] = stream.handle;
        changed_.notify_all();
    }
    else if (event->Type == QUIC_STREAM_EVENT_RECEIVE)
    {
        std::lock_guard lock(mutex_);
        receive(stream, event->RECEIVE.Buffers,
                event->RECEIVE.BufferCount, false);
        changed_.notify_all();
    }
    else if (event->Type == QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN)
    {
        std::lock_guard lock(mutex_);
        receive(stream, nullptr, 0, true);
        changed_.notify_all();
    }
    else if (event->Type == QUIC_STREAM_EVENT_SEND_COMPLETE)
    {
        std::unique_ptr<send_state> sent(static_cast<send_state*>(
            event->SEND_COMPLETE.ClientContext));
    }
    else if (event->Type == QUIC_STREAM_EVENT_PEER_SEND_ABORTED ||
             event->Type == QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE)
    {
        std::lock_guard lock(mutex_);
        if (stream.id == response_stream_ && !response_done_ && !error_)
        {
            try
            {
                network::fail("connection_closed", "HTTP/3 响应流提前关闭");
            }
            catch (...)
            {
                error_ = std::current_exception();
            }
        }
        changed_.notify_all();
    }
}

void client_connection::pump()
{
    // 调用方持有 mutex_，保证 QPACK、流表及 MsQuic 发送顺序一致。
    while (true)
    {
        std::int64_t stream_id = -1;
        int fin = 0;
        nghttp3_vec vectors[8]{};
        const auto count = nghttp3_conn_writev_stream(h3_, &stream_id,
            &fin, vectors, 8);
        if (count < 0)
        {
            network::fail("protocol_error", "生成 HTTP/3 发送帧失败");
        }
        if (stream_id < 0)
        {
            return;
        }
        const auto found = stream_handles_.find(stream_id);
        if (found == stream_handles_.end())
        {
            network::fail("protocol_error", "HTTP/3 输出流未绑定 QUIC 句柄");
        }
        auto sent = std::make_unique<send_state>();
        sent->stream_id = stream_id;
        std::size_t total = 0;
        for (nghttp3_ssize index = 0; index < count; ++index)
        {
            total += vectors[index].len;
        }
        for (nghttp3_ssize index = 0; index < count; ++index)
        {
            const auto take = std::min<std::size_t>(vectors[index].len,
                16 * 1024 - sent->bytes.size());
            sent->bytes.insert(sent->bytes.end(), vectors[index].base,
                               vectors[index].base + take);
            if (sent->bytes.size() == 16 * 1024)
            {
                break;
            }
        }
        if (sent->bytes.empty() && !fin)
        {
            return;
        }
        const auto amount = sent->bytes.size();
        sent->buffer = {static_cast<std::uint32_t>(amount),
                        sent->bytes.empty() ? nullptr : sent->bytes.data()};
        const auto flags = fin && amount == total
            ? QUIC_SEND_FLAG_FIN : QUIC_SEND_FLAG_NONE;
        require_quic(api_->StreamSend(found->second, &sent->buffer, 1,
            flags, sent.get()), "发送 HTTP/3 数据");
        sent.release();
        if (nghttp3_conn_add_write_offset(h3_, stream_id, amount) != 0)
        {
            network::fail("protocol_error", "推进 HTTP/3 发送偏移失败");
        }
    }
}

} // namespace tx_generated::http3
