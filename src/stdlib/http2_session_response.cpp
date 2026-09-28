#include "stdlib/http2_session.hpp"
#include "stdlib/error.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <thread>

namespace tx_generated::http2
{

std::vector<std::pair<std::string, std::string>>
server_session::response_headers(const http_response_data& value,
                                 std::int64_t body_length)
{
    if (value.status < 100 || value.status > 599 || body_length < 0)
    {
        network::fail("invalid_argument", "HTTP/2 响应状态码或长度无效");
    }
    if ((value.status < 200 || value.status == 204 || value.status == 304) &&
        body_length != 0)
    {
        network::fail("invalid_argument", "此状态码不能携带 HTTP/2 正文");
    }
    std::vector<std::pair<std::string, std::string>> result;
    result.emplace_back(":status", std::to_string(value.status));
    std::size_t header_bytes = result.front().first.size() +
                               result.front().second.size();
    for (const auto& [name, content] : value.headers)
    {
        network::validate_header(name, content);
        const auto key = network::lower_ascii(name);
        if (key == "content-length" || key == "transfer-encoding" ||
            key == "connection" || key == "keep-alive" ||
            key == "proxy-connection" || key == "upgrade" ||
            key == "set-cookie")
        {
            network::fail("invalid_header", "HTTP/2 响应包含保留头字段");
        }
        header_bytes += key.size() + content.size();
        result.emplace_back(key, content);
    }
    for (const auto& cookie : value.cookies)
    {
        network::validate_header("set-cookie", cookie);
        header_bytes += 10 + cookie.size();
        result.emplace_back("set-cookie", cookie);
    }
    const auto length_text = std::to_string(body_length);
    header_bytes += 14 + length_text.size();
    if (header_bytes > network::max_head_bytes)
    {
        network::fail("size_limit", "HTTP/2 响应头超过 64 KiB");
    }
    result.emplace_back("content-length", length_text);
    return result;
}

nghttp2_ssize server_session::read_response_data(nghttp2_session*,
    std::int32_t, std::uint8_t* buffer, std::size_t length,
    std::uint32_t* flags, nghttp2_data_source* source, void* data)
{
    auto* self = static_cast<server_session*>(data);
    auto* stream = static_cast<request_state*>(source->ptr);
    try
    {
        if (stream->response_remaining == 0)
        {
            *flags |= NGHTTP2_DATA_FLAG_EOF;
            return 0;
        }
        const auto amount = static_cast<std::size_t>(
            std::min<std::uint64_t>(length, stream->response_remaining));
        std::size_t copied = 0;
        if (stream->response_source)
        {
            copied = stream->response_source->file.read_into(
                reinterpret_cast<char*>(buffer), amount);
            if (copied == 0)
            {
                network::fail("operation_failed", "HTTP/2 响应源流提前结束");
            }
        }
        else
        {
            copied = amount;
            std::memcpy(buffer, stream->response_body.data() +
                        stream->response_offset, copied);
            stream->response_offset += copied;
        }
        stream->response_remaining -= copied;
        if (stream->response_remaining == 0)
        {
            *flags |= NGHTTP2_DATA_FLAG_EOF;
        }
        return static_cast<nghttp2_ssize>(copied);
    }
    catch (...)
    {
        self->record_error();
        return NGHTTP2_ERR_CALLBACK_FAILURE;
    }
}

void server_session::respond(const std::shared_ptr<request_state>& stream,
                             const http_response_data& value,
                             const binary_stream& source,
                             std::int64_t body_length, bool binary)
{
    std::unique_lock io_lock(io_mutex_);
    if (!stream || !stream->complete || stream->response_done)
    {
        network::fail("connection_closed", "HTTP/2 请求流不可回复");
    }
    if (source && body_length > 0)
    {
        source->file.require_open();
    }
    if (!source && static_cast<std::uint64_t>(body_length) != value.body.size())
    {
        network::fail("invalid_argument", "HTTP/2 响应正文长度不匹配");
    }
    if (!binary && !source)
    {
        network::validate_utf8(value.body);
    }
    auto headers = response_headers(value, body_length);
    std::vector<nghttp2_nv> fields;
    fields.reserve(headers.size());
    for (auto& [name, content] : headers)
    {
        fields.push_back({reinterpret_cast<std::uint8_t*>(name.data()),
                          reinterpret_cast<std::uint8_t*>(content.data()),
                          name.size(), content.size(), NGHTTP2_NV_FLAG_NONE});
    }
    stream->response_source = source;
    stream->response_body = value.body;
    stream->response_remaining = static_cast<std::uint64_t>(body_length);
    stream->response_offset = 0;
    nghttp2_data_provider2 provider{};
    provider.source.ptr = stream.get();
    provider.read_callback = read_response_data;
    const auto status = nghttp2_submit_response2(session_, stream->stream_id,
        fields.data(), fields.size(), body_length == 0 ? nullptr : &provider);
    if (status != 0)
    {
        network::fail("operation_failed", "提交 HTTP/2 响应失败");
    }
    const auto deadline = std::chrono::steady_clock::now() +
        std::chrono::seconds(30);
    while (!stream->response_done)
    {
        flush_output();
        if (stream->response_done)
        {
            break;
        }
        if (!nghttp2_session_want_read(session_))
        {
            network::fail("connection_closed", "HTTP/2 响应流提前关闭");
        }
        if (std::chrono::steady_clock::now() >= deadline)
        {
            network::fail("timeout", "HTTP/2 响应等待流控超时");
        }
        io_.set_receive_timeout(50);
        try
        {
            if (!read_input())
            {
                network::fail("connection_closed", "HTTP/2 响应发送时连接已关闭");
            }
        }
        catch (const runtime_failure& error)
        {
            if (error.error().code != "timeout")
            {
                throw;
            }
        }
        io_lock.unlock();
        std::this_thread::yield();
        io_lock.lock();
    }
    streams_.erase(stream->stream_id);
}

void server_session::close_stream(
    const std::shared_ptr<request_state>& stream) noexcept
{
    if (!stream)
    {
        return;
    }
    try
    {
        std::lock_guard io_lock(io_mutex_);
        if (!stream->response_done)
        {
            nghttp2_submit_rst_stream(session_, NGHTTP2_FLAG_NONE,
                stream->stream_id, NGHTTP2_CANCEL);
            flush_output();
        }
    }
    catch (...)
    {
    }
    streams_.erase(stream->stream_id);
}

} // namespace tx_generated::http2
