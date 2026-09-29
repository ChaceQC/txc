#include "stdlib/http3_server_internal.hpp"

#include "stdlib/error.hpp"

#include <algorithm>
#include <charconv>
#include <cstring>
#include <utility>

namespace tx_generated::http3
{

QUIC_STATUS QUIC_API server_connection::on_connection(HQUIC, void* context,
                                                        QUIC_CONNECTION_EVENT* event)
{
    auto* self = static_cast<server_connection*>(context);
    const auto keep_alive = self->weak_from_this().lock();
    (void)keep_alive;
    {
        std::lock_guard lock(self->mutex_);
        ++self->connection_callbacks_active_;
    }
    QUIC_STATUS result = QUIC_STATUS_SUCCESS;
    try
    {
        self->connection_event(event);
    }
    catch (...)
    {
        self->capture_error();
        self->shutdown();
        result = QUIC_STATUS_INTERNAL_ERROR;
    }
    {
        std::lock_guard lock(self->mutex_);
        --self->connection_callbacks_active_;
        self->changed_.notify_all();
    }
    return result;
}

void server_connection::open_control_streams()
{
    for (const auto kind : {stream_state::role::control,
                            stream_state::role::encoder,
                            stream_state::role::decoder})
    {
        auto state = std::make_unique<stream_state>();
        state->kind = kind;
        require_quic(api_->StreamOpen(connection_,
            QUIC_STREAM_OPEN_FLAG_UNIDIRECTIONAL, on_stream,
            this, &state->handle), "打开 HTTP/3 服务端控制流");
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
        state->handle = event->PEER_STREAM_STARTED.Stream;
        std::uint32_t size = sizeof(state->id);
        require_quic(api_->GetParam(state->handle, QUIC_PARAM_STREAM_ID,
            &size, &state->id), "读取 HTTP/3 请求流编号");
        // QUIC ID 的低两位分别标识发起方和方向。客户端双向流承载
        // 请求；客户端单向流交给 nghttp3 识别控制/QPACK 或扩展流。
        state->kind = (state->id & 0x3) == 0
            ? stream_state::role::peer_request
            : stream_state::role::peer_unidirectional;
        state->started = true;
        auto* raw = state.get();
        {
            std::lock_guard lock(mutex_);
            if (!stream_handles_.emplace(state->id, state->handle).second)
            {
                network::fail("protocol_error", "HTTP/3 对端流编号重复");
            }
            streams_.push_back(std::move(state));
        }
        api_->SetCallbackHandler(raw->handle,
            reinterpret_cast<void*>(on_stream), this);
    }
    else if (event->Type == QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT ||
             event->Type == QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER)
    {
        std::unordered_map<std::int64_t, std::shared_ptr<server_request>>
            retired;
        {
            std::lock_guard lock(mutex_);
            shutdown_ = true;
            shutdown_requested_ = true;
            for (auto& [id, request] : requests_)
            {
                request->closed = true;
            }
            retired.swap(requests_);
            changed_.notify_all();
        }
        if (const auto listener = listener_.lock())
        {
            listener->discard(this);
        }
    }
    else if (event->Type == QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE)
    {
        std::unordered_map<std::int64_t, std::shared_ptr<server_request>>
            retired;
        {
            std::lock_guard lock(mutex_);
            shutdown_ = true;
            shutdown_requested_ = true;
            listener_closed_ = true;
            connection_shutdown_complete_ = true;
            for (auto& [id, request] : requests_)
            {
                request->closed = true;
            }
            retired.swap(requests_);
            changed_.notify_all();
        }
        if (const auto listener = listener_.lock())
        {
            listener->discard(this);
        }
    }
}

QUIC_STATUS QUIC_API server_connection::on_stream(HQUIC handle, void* context,
                                                    QUIC_STREAM_EVENT* event)
{
    auto* owner = static_cast<server_connection*>(context);
    const auto keep_alive = owner->weak_from_this().lock();
    (void)keep_alive;
    stream_state* state = nullptr;
    {
        std::lock_guard lock(owner->mutex_);
        ++owner->stream_callbacks_active_;
        const auto found = std::find_if(owner->streams_.begin(),
            owner->streams_.end(), [handle](const auto& value)
            {
                return value->handle == handle;
            });
        if (found != owner->streams_.end())
        {
            state = found->get();
            ++state->callbacks_active;
        }
    }
    if (!state)
    {
        if (event->Type == QUIC_STREAM_EVENT_SEND_COMPLETE)
        {
            std::unique_ptr<send_state> sent(static_cast<send_state*>(
                event->SEND_COMPLETE.ClientContext));
        }
        {
            std::lock_guard lock(owner->mutex_);
            --owner->stream_callbacks_active_;
            owner->changed_.notify_all();
        }
        return QUIC_STATUS_SUCCESS;
    }
    QUIC_STATUS result = QUIC_STATUS_SUCCESS;
    try
    {
        owner->stream_event(*state, event);
    }
    catch (...)
    {
        owner->fail_stream(*state);
        owner->capture_error();
        owner->shutdown();
        result = QUIC_STATUS_INTERNAL_ERROR;
    }
    if (event->Type == QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE)
    {
        // SHUTDOWN_COMPLETE 是该流最后一次回调。统一在事件处理逻辑退出
        // 后回收其上下文，避免回调仍在使用 stream_state 时释放它。
        owner->finish_stream_shutdown(*state, event);
    }
    owner->release_stream_callback(*state);
    return result;
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
    if (fin && !stream.peer_send_shutdown &&
        nghttp3_conn_read_stream(h3_, stream.id, nullptr, 0, 1) < 0)
    {
        network::fail("protocol_error", "HTTP/3 请求结束帧无效");
    }
    if (fin)
    {
        stream.peer_send_shutdown = true;
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
            event->RECEIVE.BufferCount,
            (event->RECEIVE.Flags & QUIC_RECEIVE_FLAG_FIN) != 0);
    }
    else if (event->Type == QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN)
    {
        std::lock_guard lock(mutex_);
        receive(stream, nullptr, 0, true);
    }
    else if (event->Type == QUIC_STREAM_EVENT_PEER_SEND_ABORTED ||
             event->Type == QUIC_STREAM_EVENT_PEER_RECEIVE_ABORTED)
    {
        HQUIC handle = nullptr;
        std::uint64_t error_code = event->Type ==
            QUIC_STREAM_EVENT_PEER_SEND_ABORTED
            ? event->PEER_SEND_ABORTED.ErrorCode
            : event->PEER_RECEIVE_ABORTED.ErrorCode;
        std::shared_ptr<server_request> retired;
        {
            std::lock_guard lock(mutex_);
            if (stream.closure == stream_state::close_reason::active)
            {
                stream.closure = stream_state::close_reason::peer_abort;
            }
            stream.peer_error_code = error_code;
            if (stream.kind == stream_state::role::peer_request)
            {
                const auto found = requests_.find(stream.id);
                if (found != requests_.end())
                {
                    found->second->closed = true;
                    retired = std::move(found->second);
                    requests_.erase(found);
                }
            }
            if (stream.kind == stream_state::role::peer_request &&
                !stream.abort_requested && !shutdown_ && !shutdown_requested_)
            {
                stream.abort_requested = true;
                handle = stream.handle;
            }
            changed_.notify_all();
        }
        if (stream.kind == stream_state::role::peer_request)
        {
            if (const auto listener = listener_.lock())
            {
                listener->discard(this, stream.id);
            }
        }
        if (handle)
        {
            api_->StreamShutdown(handle,
                QUIC_STREAM_SHUTDOWN_FLAG_ABORT_SEND |
                QUIC_STREAM_SHUTDOWN_FLAG_ABORT_RECEIVE,
                error_code);
        }
    }
    else if (event->Type == QUIC_STREAM_EVENT_SEND_SHUTDOWN_COMPLETE)
    {
        std::lock_guard lock(mutex_);
        stream.send_shutdown_complete = true;
    }
    else if (event->Type == QUIC_STREAM_EVENT_SEND_COMPLETE)
    {
        std::unique_ptr<send_state> sent(static_cast<send_state*>(
            event->SEND_COMPLETE.ClientContext));
        std::lock_guard lock(mutex_);
        const auto state = std::find_if(streams_.begin(), streams_.end(),
            [id = sent->stream_id](const auto& value)
            {
                return value->id == id;
            });
        if (state != streams_.end() && (*state)->pending_sends > 0)
        {
            --(*state)->pending_sends;
        }
        if (sent->fin && !event->SEND_COMPLETE.Canceled)
        {
            const auto found = requests_.find(sent->stream_id);
            if (found != requests_.end())
            {
                found->second->response_done = true;
                changed_.notify_all();
            }
        }
    }
}

void server_connection::finish_stream_shutdown(stream_state& stream,
                                               QUIC_STREAM_EVENT* event) noexcept
{
    std::shared_ptr<server_request> retired_request;
    std::int64_t id = stream.id;
    int nghttp3_status = 0;
    bool should_shutdown = false;
    {
        std::lock_guard lock(mutex_);
        if (stream.shutdown_complete)
        {
            return;
        }
        stream.shutdown_complete = true;
        if (event->SHUTDOWN_COMPLETE.ConnectionShutdown || shutdown_ ||
            shutdown_requested_ ||
            connection_shutdown_complete_)
        {
            stream.closure = stream_state::close_reason::connection_closed;
        }
        else if (stream.closure == stream_state::close_reason::active)
        {
            stream.closure = stream_state::close_reason::normal;
        }

        const auto stream_handle = stream_handles_.find(id);
        if (stream_handle != stream_handles_.end() &&
            stream_handle->second == stream.handle)
        {
            stream_handles_.erase(stream_handle);
        }

        const auto request = requests_.find(id);
        if (request != requests_.end())
        {
            if (stream.closure == stream_state::close_reason::normal &&
                stream.response_fin_submitted)
            {
                request->second->response_done = true;
            }
            if (!request->second->response_done)
            {
                request->second->closed = true;
            }
            retired_request = std::move(request->second);
            requests_.erase(request);
            changed_.notify_all();
        }

        if (h3_ && id >= 0 &&
            stream.closure != stream_state::close_reason::connection_closed)
        {
            nghttp3_status = nghttp3_conn_close_stream(h3_, id,
                stream.closure == stream_state::close_reason::peer_abort
                    ? stream.peer_error_code : 0);
            should_shutdown = nghttp3_status ==
                NGHTTP3_ERR_H3_CLOSED_CRITICAL_STREAM;
        }
        changed_.notify_all();
    }

    if (const auto listener = listener_.lock())
    {
        listener->discard(this, id);
    }
    if (nghttp3_status != 0 && nghttp3_status != NGHTTP3_ERR_STREAM_NOT_FOUND &&
        nghttp3_status != NGHTTP3_ERR_H3_CLOSED_CRITICAL_STREAM)
    {
        try
        {
            network::fail("protocol_error", "回收 HTTP/3 流状态失败");
        }
        catch (...)
        {
            capture_error();
        }
        should_shutdown = true;
    }
    if (should_shutdown)
    {
        shutdown();
    }
}

void server_connection::release_stream_callback(stream_state& stream) noexcept
{
    {
        std::lock_guard lock(mutex_);
        if (stream.callbacks_active > 0)
        {
            --stream.callbacks_active;
        }
        if (stream_callbacks_active_ > 0)
        {
            --stream_callbacks_active_;
        }
        changed_.notify_all();
    }
    reap_closed_streams();
}

void server_connection::reap_closed_streams() noexcept
{
    while (true)
    {
        std::unique_ptr<stream_state> retired;
        HQUIC handle = nullptr;
        {
            std::lock_guard lock(mutex_);
            const auto ready = std::find_if(streams_.begin(), streams_.end(),
                [](const auto& value)
                {
                    return value->shutdown_complete &&
                        value->callbacks_active == 0 &&
                        value->native_operations_active == 0 &&
                        value->pending_sends == 0 && !value->close_called;
                });
            if (ready == streams_.end())
            {
                return;
            }
            (*ready)->close_called = true;
            handle = std::exchange((*ready)->handle, nullptr);
            retired = std::move(*ready);
            streams_.erase(ready);
            ++stream_closes_active_;
        }
        // 句柄关闭在锁外执行；全局计数保证它完成前连接仍存活。
        if (handle)
        {
            api_->StreamClose(handle);
        }
        {
            std::lock_guard lock(mutex_);
            --stream_closes_active_;
            changed_.notify_all();
        }
    }
}

void server_connection::fail_stream(stream_state& stream) noexcept
{
    std::shared_ptr<server_request> retired;
    HQUIC handle = nullptr;
    std::int64_t id = stream.id;
    {
        std::lock_guard lock(mutex_);
        if (stream.closure == stream_state::close_reason::active)
        {
            stream.closure = stream_state::close_reason::local_abort;
        }
        if (id >= 0)
        {
            const auto found = requests_.find(id);
            if (found != requests_.end())
            {
                found->second->closed = true;
                retired = std::move(found->second);
                requests_.erase(found);
            }
        }
        if (stream.kind == stream_state::role::peer_request &&
            !stream.abort_requested && !shutdown_ && !shutdown_requested_)
        {
            stream.abort_requested = true;
            handle = stream.handle;
        }
        changed_.notify_all();
    }
    if (handle)
    {
        api_->StreamShutdown(handle,
            QUIC_STREAM_SHUTDOWN_FLAG_ABORT_SEND |
            QUIC_STREAM_SHUTDOWN_FLAG_ABORT_RECEIVE, 0);
    }
    try
    {
        if (const auto listener = listener_.lock())
        {
            listener->discard(this, id < 0 ? -1 : id);
        }
    }
    catch (...)
    {
        // 清理路径不能覆盖原始协议错误。
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
        const auto state = std::find_if(streams_.begin(), streams_.end(),
            [stream_id](const auto& value)
            {
                return value->id == stream_id;
            });
        if (state == streams_.end())
        {
            network::fail("protocol_error", "HTTP/3 响应流上下文已回收");
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
        // send_state 拥有缓冲区直到 SEND_COMPLETE；最终 SHUTDOWN_COMPLETE
        // 到达时，所有发送回调和对应缓冲都必须已经收束。
        ++(*state)->pending_sends;
        const auto status = api_->StreamSend(found->second, &sent->buffer, 1,
            sent->fin ? QUIC_SEND_FLAG_FIN : QUIC_SEND_FLAG_NONE, sent.get());
        if (QUIC_FAILED(status))
        {
            --(*state)->pending_sends;
            require_quic(status, "发送 HTTP/3 响应");
        }
        if (sent->fin)
        {
            (*state)->response_fin_submitted = true;
        }
        sent.release();
        if (nghttp3_conn_add_write_offset(h3_, stream_id, amount) != 0)
        {
            network::fail("protocol_error", "推进 HTTP/3 响应偏移失败");
        }
    }
}

} // namespace tx_generated::http3
