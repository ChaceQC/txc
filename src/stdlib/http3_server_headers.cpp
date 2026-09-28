#include "stdlib/http3_server_internal.hpp"

#include "stdlib/error.hpp"

#include <charconv>

namespace tx_generated::http3
{

int server_connection::on_begin_headers(nghttp3_conn*, std::int64_t id,
                                         void* context, void*)
{
    auto* self = static_cast<server_connection*>(context);
    try
    {
        if (self->requests_.size() >= 16 || self->requests_.contains(id))
        {
            network::fail("size_limit", "HTTP/3 并发请求流超过 16 条");
        }
        auto request = std::make_shared<server_request>();
        request->session = self->shared_from_this();
        request->stream_id = id;
        self->requests_.emplace(id, std::move(request));
        return 0;
    }
    catch (...)
    {
        self->error_ = std::current_exception();
        self->changed_.notify_all();
        return NGHTTP3_ERR_CALLBACK_FAILURE;
    }
}

int server_connection::on_header(nghttp3_conn*, std::int64_t id,
    std::int32_t, nghttp3_rcbuf* name, nghttp3_rcbuf* value,
    std::uint8_t, void* context, void*)
{
    auto* self = static_cast<server_connection*>(context);
    try
    {
        auto& request = *self->requests_.at(id);
        const auto name_bytes = nghttp3_rcbuf_get_buf(name);
        const auto value_bytes = nghttp3_rcbuf_get_buf(value);
        const std::string_view key(reinterpret_cast<const char*>(
            name_bytes.base), name_bytes.len);
        const std::string_view data(reinterpret_cast<const char*>(
            value_bytes.base), value_bytes.len);
        request.header_bytes += key.size() + data.size();
        if (request.header_bytes > network::max_head_bytes)
        {
            network::fail("size_limit", "HTTP/3 请求头超过 64 KiB");
        }
        if (key == ":method")
        {
            request.value.method = data;
        }
        else if (key == ":path")
        {
            request.value.target = data;
        }
        else if (key == ":scheme")
        {
            request.scheme = data;
        }
        else if (key == ":authority")
        {
            request.authority = data;
        }
        else
        {
            network::validate_header(key, data);
            if (key != network::lower_ascii(key) || key == "connection" ||
                key == "transfer-encoding" || key == "set-cookie")
            {
                network::fail("protocol_error", "HTTP/3 请求头名称无效");
            }
            auto [item, inserted] = request.value.headers.emplace(key, data);
            if (!inserted)
            {
                if (key == "content-length" || key == "host")
                {
                    network::fail("protocol_error", "HTTP/3 请求头重复");
                }
                item->second += key == "cookie" ? "; " : ", ";
                item->second += data;
            }
            if (key == "content-length")
            {
                (void)network::content_length(request.value.headers,
                                              network::max_body_bytes);
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

int server_connection::on_data(nghttp3_conn*, std::int64_t id,
    const std::uint8_t* data, std::size_t length, void* context, void*)
{
    auto* self = static_cast<server_connection*>(context);
    try
    {
        auto& request = *self->requests_.at(id);
        if (length > network::max_body_bytes - request.value.body.size())
        {
            network::fail("size_limit", "HTTP/3 请求正文超过 8 MiB");
        }
        request.value.body.append(reinterpret_cast<const char*>(data), length);
        request.value.body_length += static_cast<std::int64_t>(length);
        return 0;
    }
    catch (...)
    {
        self->error_ = std::current_exception();
        self->changed_.notify_all();
        return NGHTTP3_ERR_CALLBACK_FAILURE;
    }
}

int server_connection::on_end_stream(nghttp3_conn*, std::int64_t id,
                                      void* context, void*)
{
    auto* self = static_cast<server_connection*>(context);
    try
    {
        auto request = self->requests_.at(id);
        network::validate_token(request->value.method, "HTTP/3 方法");
        network::validate_utf8(request->value.target);
        if (request->scheme != "https" || request->authority.empty() ||
            request->value.target.empty() || request->value.target.front() != '/')
        {
            network::fail("protocol_error", "HTTP/3 请求伪头字段无效");
        }
        if (request->value.headers.contains("content-length") &&
            network::content_length(request->value.headers,
                network::max_body_bytes) != request->value.body.size())
        {
            network::fail("protocol_error", "HTTP/3 正文长度与声明不符");
        }
        if (const auto host = request->value.headers.find("host");
            host != request->value.headers.end() &&
            host->second != request->authority)
        {
            network::fail("protocol_error", "HTTP/3 Host 与 authority 不一致");
        }
        request->value.headers.emplace("host", request->authority);
        request->complete = true;
        self->listener_->queue(std::move(request));
        return 0;
    }
    catch (...)
    {
        self->error_ = std::current_exception();
        self->changed_.notify_all();
        return NGHTTP3_ERR_CALLBACK_FAILURE;
    }
}

nghttp3_ssize server_connection::read_body(nghttp3_conn*, std::int64_t id,
    nghttp3_vec* vectors, std::size_t count, std::uint32_t* flags,
    void* context, void*)
{
    auto* self = static_cast<server_connection*>(context);
    const auto found = self->requests_.find(id);
    if (found == self->requests_.end())
    {
        return NGHTTP3_ERR_CALLBACK_FAILURE;
    }
    auto& request = *found->second;
    if (request.body_offered || request.response_body.empty() || count == 0)
    {
        *flags |= NGHTTP3_DATA_FLAG_EOF;
        return 0;
    }
    vectors[0] = {reinterpret_cast<std::uint8_t*>(
                      request.response_body.data()),
                  request.response_body.size()};
    request.body_offered = true;
    *flags |= NGHTTP3_DATA_FLAG_EOF;
    return 1;
}

} // namespace tx_generated::http3
