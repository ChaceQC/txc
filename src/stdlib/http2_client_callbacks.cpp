#include "stdlib/http2_client.hpp"

#include <algorithm>
#include <charconv>
#include <cstring>

namespace tx_generated::http2
{

void client_session::record_error() noexcept
{
    if (!error_)
    {
        error_ = std::current_exception();
    }
}

void client_session::check_error()
{
    if (error_)
    {
        std::rethrow_exception(error_);
    }
}

int client_session::on_header(nghttp2_session*, const nghttp2_frame* frame,
                              const std::uint8_t* name, std::size_t name_length,
                              const std::uint8_t* value, std::size_t value_length,
                              std::uint8_t, void* data)
{
    auto* self = static_cast<client_session*>(data);
    try
    {
        if (frame->hd.stream_id != self->stream_id_ ||
            frame->headers.cat != NGHTTP2_HCAT_RESPONSE)
        {
            return 0;
        }
        self->header({reinterpret_cast<const char*>(name), name_length},
                     {reinterpret_cast<const char*>(value), value_length});
        return 0;
    }
    catch (...)
    {
        self->record_error();
        return NGHTTP2_ERR_CALLBACK_FAILURE;
    }
}

int client_session::on_data_chunk(nghttp2_session*, std::uint8_t,
                                  std::int32_t stream_id,
                                  const std::uint8_t* bytes,
                                  std::size_t length, void* data)
{
    auto* self = static_cast<client_session*>(data);
    try
    {
        if (stream_id == self->stream_id_)
        {
            self->data_chunk({reinterpret_cast<const char*>(bytes), length});
        }
        return 0;
    }
    catch (...)
    {
        self->record_error();
        return NGHTTP2_ERR_CALLBACK_FAILURE;
    }
}

int client_session::on_frame_recv(nghttp2_session*,
                                  const nghttp2_frame* frame, void* data)
{
    auto* self = static_cast<client_session*>(data);
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

nghttp2_ssize client_session::read_request_data(nghttp2_session*,
    std::int32_t, std::uint8_t* buffer, std::size_t length,
    std::uint32_t* flags, nghttp2_data_source*, void* data)
{
    auto* self = static_cast<client_session*>(data);
    try
    {
        if (self->request_remaining_ == 0)
        {
            *flags |= NGHTTP2_DATA_FLAG_EOF;
            return 0;
        }
        const auto amount = static_cast<std::size_t>(
            std::min<std::uint64_t>(length, self->request_remaining_));
        std::size_t copied = 0;
        if (self->source_)
        {
            copied = self->source_->file.read_into(
                reinterpret_cast<char*>(buffer), amount);
            if (copied == 0)
            {
                network::fail("operation_failed", "HTTP/2 请求源流提前结束");
            }
        }
        else
        {
            copied = amount;
            std::memcpy(buffer, self->body_.data() + self->request_offset_,
                        copied);
            self->request_offset_ += copied;
        }
        self->request_remaining_ -= copied;
        if (self->request_remaining_ == 0)
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

void client_session::header(std::string_view name, std::string_view value)
{
    if (name.size() + value.size() >
        network::max_head_bytes - header_bytes_)
    {
        network::fail("size_limit", "HTTP/2 响应头超过 64 KiB");
    }
    header_bytes_ += name.size() + value.size();
    network::validate_utf8(value);
    if (name == ":status")
    {
        std::int64_t status = 0;
        const auto [end, error] = std::from_chars(value.data(),
            value.data() + value.size(), status);
        if (error != std::errc{} || end != value.data() + value.size() ||
            status < 100 || status > 599)
        {
            network::fail("protocol_error", "HTTP/2 响应状态码无效");
        }
        response_.status = status;
        return;
    }
    if (name.starts_with(':') || name != network::lower_ascii(name))
    {
        network::fail("protocol_error", "HTTP/2 响应头名称无效");
    }
    network::validate_header(name, value);
    if (name == "connection" || name == "transfer-encoding" ||
        name == "upgrade")
    {
        network::fail("protocol_error", "HTTP/2 响应包含禁用头字段");
    }
    if (name == "set-cookie")
    {
        response_.cookies.emplace_back(value);
        return;
    }
    auto [item, inserted] = response_.headers.emplace(name, value);
    if (!inserted)
    {
        if (name == "content-length")
        {
            network::fail("protocol_error", "HTTP/2 Content-Length 重复");
        }
        item->second += ", ";
        item->second += value;
    }
}

void client_session::data_chunk(std::string_view bytes)
{
    if (bytes.size() > static_cast<std::uint64_t>(
        max_response_bytes_ - response_.body_length))
    {
        network::fail("size_limit", "HTTP/2 响应正文超过接收上限");
    }
    if (destination_)
    {
        destination_->file.write(bytes);
    }
    else
    {
        response_.body.append(bytes);
    }
    response_.body_length += static_cast<std::int64_t>(bytes.size());
}

void client_session::frame_received(const nghttp2_frame* frame)
{
    if (frame->hd.stream_id != stream_id_)
    {
        return;
    }
    if (frame->hd.type == NGHTTP2_RST_STREAM)
    {
        network::fail("protocol_error", "HTTP/2 服务端重置了请求流");
    }
    if ((frame->hd.type == NGHTTP2_HEADERS || frame->hd.type == NGHTTP2_DATA) &&
        (frame->hd.flags & NGHTTP2_FLAG_END_STREAM) != 0)
    {
        complete_ = true;
    }
}

} // namespace tx_generated::http2
