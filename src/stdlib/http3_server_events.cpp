#include "stdlib/http3_server_internal.hpp"

#include "stdlib/error.hpp"

#include <algorithm>
#include <charconv>
#include <cstring>

namespace tx_generated::http3
{

QUIC_STATUS QUIC_API server_connection::on_connection(HQUIC, void* context,
                                                        QUIC_CONNECTION_EVENT* event)
{
    auto* self = static_cast<server_connection*>(context);
    try
    {
        self->connection_event(event);
        return QUIC_STATUS_SUCCESS;
    }
    catch (...)
    {
        self->capture_error();
        return QUIC_STATUS_INTERNAL_ERROR;
    }
}

void server_connection::open_control_streams()
{
    for (const auto kind : {stream_state::role::control,
                            stream_state::role::encoder,
                            stream_state::role::decoder})
    {
        auto state = std::make_unique<stream_state>();
        state->owner = this;
        state->kind = kind;
        require_quic(api_->StreamOpen(connection_,
            QUIC_STREAM_OPEN_FLAG_UNIDIRECTIONAL, on_stream,
            state.get(), &state->handle), "打开 HTTP/3 服务端控制流");
        auto* raw = state.get();
        {
            std::lock_guard lock(mutex_);
            streams_.push_back(std::move(state));
        }
        require_quic(api_->StreamStart(raw->handle,
            QUIC_STREAM_START_FLAG_IMMEDIATE), "启动 HTTP/3 服务端控制流");
    }
}

void server_connection::bind_control_streams()
{
    if (bound_ || control_id_ < 0 || encoder_id_ < 0 || decoder_id_ < 0)
    {
        return;
    }
    if (nghttp3_conn_bind_control_stream(h3_, control_id_) != 0 ||
        nghttp3_conn_bind_qpack_streams(h3_, encoder_id_, decoder_id_) != 0)
    {
        network::fail("protocol_error", "绑定 HTTP/3 服务端控制流失败");
    }
    bound_ = true;
    pump();
}

void server_connection::connection_event(QUIC_CONNECTION_EVENT* event)
{
    if (event->Type == QUIC_CONNECTION_EVENT_CONNECTED)
    {
        open_control_streams();
    }
    else if (event->Type == QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED)
    {
        auto state = std::make_unique<stream_state>();
        state->owner = this;
        state->handle = event->PEER_STREAM_STARTED.Stream;
        std::uint32_t size = sizeof(state->id);
        require_quic(api_->GetParam(state->handle, QUIC_PARAM_STREAM_ID,
            &size, &state->id), "读取 HTTP/3 请求流编号");
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
    else if (event->Type == QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE)
    {
        std::lock_guard lock(mutex_);
        shutdown_ = true;
        changed_.notify_all();
    }
}

QUIC_STATUS QUIC_API server_connection::on_stream(HQUIC, void* context,
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

void server_connection::receive(stream_state& stream,
    const QUIC_BUFFER* buffers, std::uint32_t count, bool fin)
{
    for (std::uint32_t index = 0; index < count; ++index)
    {
        if (nghttp3_conn_read_stream(h3_, stream.id,
                buffers[index].Buffer, buffers[index].Length, 0) < 0)
        {
            network::fail("protocol_error", "HTTP/3 请求帧无效");
        }
    }
    if (fin && nghttp3_conn_read_stream(h3_, stream.id, nullptr, 0, 1) < 0)
    {
        network::fail("protocol_error", "HTTP/3 请求结束帧无效");
    }
    if (bound_)
    {
        pump();
    }
}

void server_connection::stream_event(stream_state& stream,
                                      QUIC_STREAM_EVENT* event)
{
    if (event->Type == QUIC_STREAM_EVENT_START_COMPLETE)
    {
        std::lock_guard lock(mutex_);
        require_quic(event->START_COMPLETE.Status, "启动 HTTP/3 控制流");
        stream.id = static_cast<std::int64_t>(event->START_COMPLETE.ID);
        stream.started = true;
        stream_handles_[stream.id] = stream.handle;
        if (stream.kind == stream_state::role::control)
        {
            control_id_ = stream.id;
        }
        else if (stream.kind == stream_state::role::encoder)
        {
            encoder_id_ = stream.id;
        }
        else if (stream.kind == stream_state::role::decoder)
        {
            decoder_id_ = stream.id;
        }
        bind_control_streams();
    }
    else if (event->Type == QUIC_STREAM_EVENT_RECEIVE)
    {
        std::lock_guard lock(mutex_);
        receive(stream, event->RECEIVE.Buffers,
                event->RECEIVE.BufferCount, false);
    }
    else if (event->Type == QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN)
    {
        std::lock_guard lock(mutex_);
        receive(stream, nullptr, 0, true);
    }
    else if (event->Type == QUIC_STREAM_EVENT_SEND_COMPLETE)
    {
        std::unique_ptr<send_state> sent(static_cast<send_state*>(
            event->SEND_COMPLETE.ClientContext));
        if (sent->fin && !event->SEND_COMPLETE.Canceled)
        {
            std::lock_guard lock(mutex_);
            const auto found = requests_.find(sent->stream_id);
            if (found != requests_.end())
            {
                found->second->response_done = true;
                changed_.notify_all();
            }
        }
    }
}

void server_connection::pump()
{
    // 调用方持有 mutex_，一条连接的 QPACK 与 QUIC 发送偏移只由此处推进。
    while (true)
    {
        std::int64_t stream_id = -1;
        int fin = 0;
        nghttp3_vec vectors[8]{};
        const auto count = nghttp3_conn_writev_stream(h3_, &stream_id,
            &fin, vectors, 8);
        if (count < 0)
        {
            network::fail("protocol_error", "生成 HTTP/3 响应帧失败");
        }
        if (stream_id < 0)
        {
            return;
        }
        const auto found = stream_handles_.find(stream_id);
        if (found == stream_handles_.end())
        {
            network::fail("protocol_error", "HTTP/3 响应流未绑定 QUIC 句柄");
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
        sent->fin = fin && amount == total;
        sent->buffer = {static_cast<std::uint32_t>(amount),
                        sent->bytes.empty() ? nullptr : sent->bytes.data()};
        require_quic(api_->StreamSend(found->second, &sent->buffer, 1,
            sent->fin ? QUIC_SEND_FLAG_FIN : QUIC_SEND_FLAG_NONE, sent.get()),
            "发送 HTTP/3 响应");
        sent.release();
        if (nghttp3_conn_add_write_offset(h3_, stream_id, amount) != 0)
        {
            network::fail("protocol_error", "推进 HTTP/3 响应偏移失败");
        }
    }
}

} // namespace tx_generated::http3
