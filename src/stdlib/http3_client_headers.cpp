#include "stdlib/http3_client_internal.hpp"

#include <charconv>

namespace tx_generated::http3
{

int client_connection::on_begin_headers(nghttp3_conn*, std::int64_t stream_id,
                                         void* context, void*)
{
    auto* self = static_cast<client_connection*>(context);
    if (stream_id != self->response_stream_)
    {
        return NGHTTP3_ERR_CALLBACK_FAILURE;
    }
    if (self->response_.status >= 100 && self->response_.status < 200)
    {
        // 1xx 只作为中间响应，交付最后一组非 1xx 头字段。
        self->response_.status = 0;
        self->response_.headers.clear();
        self->response_.cookies.clear();
        self->header_bytes_ = 0;
    }
    return 0;
}

int client_connection::on_header(nghttp3_conn*, std::int64_t stream_id,
    std::int32_t, nghttp3_rcbuf* name, nghttp3_rcbuf* value,
    std::uint8_t, void* context, void*)
{
    auto* self = static_cast<client_connection*>(context);
    try
    {
        if (stream_id != self->response_stream_)
        {
            network::fail("protocol_error", "HTTP/3 响应流编号无效");
        }
        const auto name_bytes = nghttp3_rcbuf_get_buf(name);
        const auto value_bytes = nghttp3_rcbuf_get_buf(value);
        const std::string_view key(reinterpret_cast<const char*>(
            name_bytes.base), name_bytes.len);
        const std::string_view data(reinterpret_cast<const char*>(
            value_bytes.base), value_bytes.len);
        self->header_bytes_ += key.size() + data.size();
        if (self->header_bytes_ > network::max_head_bytes)
        {
            network::fail("size_limit", "HTTP/3 响应头超过 64 KiB");
        }
        if (key == ":status")
        {
            std::int64_t status = 0;
            const auto [end, error] = std::from_chars(data.data(),
                data.data() + data.size(), status);
            if (self->response_.status != 0 || error != std::errc{} ||
                end != data.data() + data.size() || status < 100 ||
                status > 599)
            {
                network::fail("protocol_error", "HTTP/3 状态码无效");
            }
            self->response_.status = status;
        }
        else
        {
            network::validate_header(key, data);
            if (key != network::lower_ascii(key) ||
                key == "connection" || key == "transfer-encoding")
            {
                network::fail("protocol_error", "HTTP/3 响应头名称无效");
            }
            if (key == "set-cookie")
            {
                self->response_.cookies.emplace_back(data);
            }
            else if (auto [item, inserted] =
                self->response_.headers.emplace(key, data); !inserted)
            {
                if (key == "content-length" || key == "www-authenticate" ||
                    key == "proxy-authenticate")
                {
                    network::fail("protocol_error", "HTTP/3 响应头重复");
                }
                item->second += ", ";
                item->second += data;
            }
        }
        return 0;
    }
    catch (...)
    {
        self->error_ = std::current_exception();
        self->changed_.notify_all();
        return NGHTTP3_ERR_CALLBACK_FAILURE;
    }
}

int client_connection::on_data(nghttp3_conn*, std::int64_t stream_id,
    const std::uint8_t* data, std::size_t length, void* context, void*)
{
    auto* self = static_cast<client_connection*>(context);
    try
    {
        if (stream_id != self->response_stream_ ||
            length > network::max_body_bytes - self->response_.body.size())
        {
            network::fail("size_limit", "HTTP/3 响应正文超过 8 MiB");
        }
        self->response_.body.append(reinterpret_cast<const char*>(data), length);
        return 0;
    }
    catch (...)
    {
        self->error_ = std::current_exception();
        self->changed_.notify_all();
        return NGHTTP3_ERR_CALLBACK_FAILURE;
    }
}

int client_connection::on_end_stream(nghttp3_conn*, std::int64_t stream_id,
    void* context, void*)
{
    auto* self = static_cast<client_connection*>(context);
    try
    {
        if (stream_id == self->response_stream_)
        {
            if (self->method_ != "HEAD" && self->response_.status != 204 &&
                self->response_.status != 304 &&
                self->response_.headers.contains("content-length") &&
                network::content_length(self->response_.headers,
                    network::max_body_bytes) != self->response_.body.size())
            {
                network::fail("protocol_error", "HTTP/3 响应正文长度不符");
            }
            self->response_done_ = true;
            self->changed_.notify_all();
        }
        return 0;
    }
    catch (...)
    {
        self->error_ = std::current_exception();
        self->changed_.notify_all();
        return NGHTTP3_ERR_CALLBACK_FAILURE;
    }
}

nghttp3_ssize client_connection::read_body(nghttp3_conn*, std::int64_t,
    nghttp3_vec* vectors, std::size_t count, std::uint32_t* flags,
    void* context, void*)
{
    auto* self = static_cast<client_connection*>(context);
    if (self->body_offered_ || self->body_.empty() || count == 0)
    {
        *flags |= NGHTTP3_DATA_FLAG_EOF;
        return 0;
    }
    vectors[0] = {reinterpret_cast<std::uint8_t*>(self->body_.data()),
                  self->body_.size()};
    self->body_offered_ = true;
    *flags |= NGHTTP3_DATA_FLAG_EOF;
    return 1;
}

} // namespace tx_generated::http3
