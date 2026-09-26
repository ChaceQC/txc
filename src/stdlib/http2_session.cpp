#include "stdlib/http2_session.hpp"

#include <algorithm>
#include <cstring>

namespace tx_generated::http2
{

server_session::server_session(network::socket_handle socket,
                               const std::shared_ptr<tls_config>& tls,
                               std::int64_t timeout_ms)
    : io_(std::move(socket), tls, timeout_ms), secure_(tls != nullptr)
{
    nghttp2_session_callbacks* callbacks = nullptr;
    if (nghttp2_session_callbacks_new(&callbacks) != 0)
    {
        network::fail("operation_failed", "创建 HTTP/2 回调失败");
    }
    nghttp2_session_callbacks_set_on_begin_headers_callback(callbacks,
                                                              on_begin_headers);
    nghttp2_session_callbacks_set_on_header_callback(callbacks, on_header);
    nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks,
                                                                on_data_chunk);
    nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks,
                                                           on_frame_recv);
    nghttp2_session_callbacks_set_on_stream_close_callback(callbacks,
                                                             on_stream_close);
    const auto status = nghttp2_session_server_new(&session_, callbacks, this);
    nghttp2_session_callbacks_del(callbacks);
    if (status != 0)
    {
        network::fail("operation_failed", "创建 HTTP/2 会话失败");
    }
    const nghttp2_settings_entry settings[] = {
        {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 1}
    };
    if (nghttp2_submit_settings(session_, NGHTTP2_FLAG_NONE, settings, 1) != 0)
    {
        nghttp2_session_del(session_);
        session_ = nullptr;
        network::fail("operation_failed", "配置 HTTP/2 会话失败");
    }
}

server_session::~server_session() noexcept
{
    if (session_)
    {
        nghttp2_session_del(session_);
    }
}

void server_session::record_error() noexcept
{
    if (!callback_error_)
    {
        callback_error_ = std::current_exception();
    }
}

void server_session::check_callback_error()
{
    if (callback_error_)
    {
        std::rethrow_exception(callback_error_);
    }
}

int server_session::on_begin_headers(nghttp2_session*,
                                     const nghttp2_frame* frame, void* data)
{
    auto* self = static_cast<server_session*>(data);
    try
    {
        self->begin_headers(frame);
        return 0;
    }
    catch (...)
    {
        self->record_error();
        return NGHTTP2_ERR_CALLBACK_FAILURE;
    }
}

int server_session::on_header(nghttp2_session*, const nghttp2_frame* frame,
                              const std::uint8_t* name, std::size_t name_length,
                              const std::uint8_t* value, std::size_t value_length,
                              std::uint8_t, void* data)
{
    auto* self = static_cast<server_session*>(data);
    try
    {
        self->header(frame,
            {reinterpret_cast<const char*>(name), name_length},
            {reinterpret_cast<const char*>(value), value_length});
        return 0;
    }
    catch (...)
    {
        self->record_error();
        return NGHTTP2_ERR_CALLBACK_FAILURE;
    }
}

int server_session::on_data_chunk(nghttp2_session*, std::uint8_t,
                                  std::int32_t stream_id,
                                  const std::uint8_t* bytes,
                                  std::size_t length, void* data)
{
    auto* self = static_cast<server_session*>(data);
    try
    {
        self->data_chunk(stream_id,
            {reinterpret_cast<const char*>(bytes), length});
        return 0;
    }
    catch (...)
    {
        self->record_error();
        return NGHTTP2_ERR_CALLBACK_FAILURE;
    }
}

int server_session::on_frame_recv(nghttp2_session*,
                                  const nghttp2_frame* frame, void* data)
{
    auto* self = static_cast<server_session*>(data);
    try
    {
        self->frame_received(frame);
        return 0;
    }
    catch (...)
    {
        self->record_error();
        return NGHTTP2_ERR_CALLBACK_FAILURE;
    }
}

int server_session::on_stream_close(nghttp2_session*, std::int32_t stream_id,
                                    std::uint32_t, void* data)
{
    auto* self = static_cast<server_session*>(data);
    if (const auto found = self->streams_.find(stream_id);
        found != self->streams_.end())
    {
        found->second->response_done = true;
    }
    return 0;
}

void server_session::begin_headers(const nghttp2_frame* frame)
{
    if (frame->hd.type != NGHTTP2_HEADERS ||
        frame->headers.cat != NGHTTP2_HCAT_REQUEST)
    {
        network::fail("protocol_error", "HTTP/2 请求头类型无效");
    }
    auto state = std::make_shared<request_state>();
    state->stream_id = frame->hd.stream_id;
    if (!pending_assigned_)
    {
        state->destination = pending_destination_;
        state->max_body_bytes = pending_limit_;
        state->binary = pending_binary_;
        pending_assigned_ = true;
    }
    streams_.emplace(state->stream_id, std::move(state));
}

void server_session::header(const nghttp2_frame* frame,
                            std::string_view name, std::string_view value)
{
    auto& stream = *streams_.at(frame->hd.stream_id);
    if (name.size() + value.size() >
        network::max_head_bytes - stream.header_bytes)
    {
        network::fail("size_limit", "HTTP/2 请求头超过 64 KiB");
    }
    stream.header_bytes += name.size() + value.size();
    network::validate_utf8(value);
    if (name == ":method")
    {
        stream.request.method = value;
    }
    else if (name == ":path")
    {
        stream.request.target = value;
    }
    else if (name == ":scheme")
    {
        stream.scheme = value;
    }
    else if (name == ":authority")
    {
        stream.authority = value;
    }
    else
    {
        if (name.starts_with(':') || name != network::lower_ascii(name))
        {
            network::fail("protocol_error", "HTTP/2 头名称无效");
        }
        network::validate_header(name, value);
        if (name == "set-cookie" || name == "connection" ||
            name == "transfer-encoding" || name == "upgrade")
        {
            network::fail("protocol_error", "HTTP/2 请求包含禁用的头字段");
        }
        auto [item, inserted] = stream.request.headers.emplace(name, value);
        if (!inserted)
        {
            if (name == "content-length" || name == "host")
            {
                network::fail("protocol_error", "HTTP/2 请求头重复");
            }
            item->second += name == "cookie" ? "; " : ", ";
            item->second += value;
        }
    }
}

void server_session::data_chunk(std::int32_t stream_id,
                                std::string_view bytes)
{
    auto& stream = *streams_.at(stream_id);
    if (bytes.size() > stream.max_body_bytes -
        static_cast<std::size_t>(stream.request.body_length))
    {
        network::fail("size_limit", "HTTP/2 请求正文超过接收上限");
    }
    if (stream.destination)
    {
        stream.destination->file.write(bytes);
    }
    else
    {
        stream.request.body.append(bytes);
    }
    stream.request.body_length += static_cast<std::int64_t>(bytes.size());
}

void server_session::validate_request(request_state& stream)
{
    network::validate_token(stream.request.method, "HTTP 方法");
    if (stream.request.target.empty() || stream.request.target.front() != '/' ||
        stream.scheme != (secure_ ? "https" : "http") ||
        stream.authority.empty())
    {
        network::fail("protocol_error", "HTTP/2 请求伪头字段无效");
    }
    network::validate_utf8(stream.request.target);
    if (!stream.request.headers.contains("host"))
    {
        stream.request.headers.emplace("host", stream.authority);
    }
    if (stream.request.headers.contains("content-length"))
    {
        const auto length = network::content_length(stream.request.headers,
                                                    stream.max_body_bytes);
        if (length != static_cast<std::size_t>(stream.request.body_length))
        {
            network::fail("protocol_error", "HTTP/2 Content-Length 与正文长度不符");
        }
    }
    if (!stream.binary && !stream.destination)
    {
        network::validate_utf8(stream.request.body);
    }
}

void server_session::frame_received(const nghttp2_frame* frame)
{
    if ((frame->hd.type != NGHTTP2_HEADERS && frame->hd.type != NGHTTP2_DATA) ||
        (frame->hd.flags & NGHTTP2_FLAG_END_STREAM) == 0)
    {
        return;
    }
    const auto found = streams_.find(frame->hd.stream_id);
    if (found != streams_.end() && !found->second->complete)
    {
        validate_request(*found->second);
        found->second->complete = true;
        completed_.push_back(frame->hd.stream_id);
    }
}

void server_session::flush_output()
{
    while (true)
    {
        const std::uint8_t* bytes = nullptr;
        const auto length = nghttp2_session_mem_send2(session_, &bytes);
        check_callback_error();
        if (length < 0)
        {
            network::fail("protocol_error", "发送 HTTP/2 帧失败");
        }
        if (length == 0)
        {
            return;
        }
        io_.write_all({reinterpret_cast<const char*>(bytes),
                       static_cast<std::size_t>(length)});
    }
}

bool server_session::read_input()
{
    const auto bytes = io_.read_some(16 * 1024);
    if (bytes.empty())
    {
        return false;
    }
    std::size_t offset = 0;
    while (offset < bytes.size())
    {
        const auto consumed = nghttp2_session_mem_recv2(session_,
            reinterpret_cast<const std::uint8_t*>(bytes.data() + offset),
            bytes.size() - offset);
        check_callback_error();
        if (consumed <= 0)
        {
            network::fail("protocol_error", "接收 HTTP/2 帧失败");
        }
        offset += static_cast<std::size_t>(consumed);
    }
    flush_output();
    return true;
}

std::shared_ptr<request_state> server_session::accept(
    const binary_stream& destination, std::size_t max_body_bytes,
    bool binary, std::int64_t timeout_ms)
{
    io_.set_receive_timeout(timeout_ms);
    pending_destination_ = destination;
    pending_limit_ = max_body_bytes;
    pending_binary_ = binary;
    pending_assigned_ = false;
    flush_output();
    while (completed_.empty())
    {
        if (!read_input())
        {
            return {};
        }
    }
    const auto stream_id = completed_.front();
    completed_.pop_front();
    auto state = streams_.at(stream_id);
    if (state->request.body_length > static_cast<std::int64_t>(max_body_bytes))
    {
        network::fail("size_limit", "HTTP/2 请求正文超过接收上限");
    }
    if (destination && !state->destination)
    {
        destination->file.write(state->request.body);
        state->request.body.clear();
        state->destination = destination;
    }
    pending_destination_.reset();
    return state;
}

} // namespace tx_generated::http2
