#include "stdlib/http3_server_internal.hpp"

#include "stdlib/error.hpp"

#include <algorithm>

namespace tx_generated::http3
{

server_connection::server_connection(
    std::shared_ptr<server_listener> listener, HQUIC connection)
    : listener_(std::move(listener)), api_(quic().get()),
      connection_(connection)
{
    nghttp3_callbacks callbacks{};
    callbacks.begin_headers = on_begin_headers;
    callbacks.recv_header = on_header;
    callbacks.recv_data = on_data;
    callbacks.end_stream = on_end_stream;
    nghttp3_settings settings;
    nghttp3_settings_default(&settings);
    settings.max_field_section_size = network::max_head_bytes;
    settings.qpack_max_dtable_capacity = 0;
    if (nghttp3_conn_server_new(&h3_, &callbacks, &settings,
                                 nullptr, this) != 0)
    {
        network::fail("operation_failed", "初始化 HTTP/3 服务端帧处理失败");
    }
    api_->SetCallbackHandler(connection_,
        reinterpret_cast<void*>(on_connection), this);
}

server_connection::~server_connection() noexcept
{
    if (connection_)
    {
        api_->ConnectionShutdown(connection_,
            QUIC_CONNECTION_SHUTDOWN_FLAG_NONE, 0);
        {
            std::unique_lock lock(mutex_);
            changed_.wait_for(lock, std::chrono::seconds(2), [&]
            {
                return shutdown_.load();
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
}

void server_connection::reject_failed_configuration() noexcept
{
    // 监听回调返回失败时，连接句柄由 MsQuic 回收，避免再次 ConnectionClose。
    connection_ = nullptr;
    shutdown_ = true;
}

void server_connection::capture_error() noexcept
{
    std::exception_ptr current = std::current_exception();
    {
        std::lock_guard lock(mutex_);
        if (!error_)
        {
            error_ = current;
        }
        changed_.notify_all();
    }
    listener_->report_error(current);
}

void server_connection::check_error()
{
    if (error_)
    {
        std::rethrow_exception(error_);
    }
}

std::vector<std::pair<std::string, std::string>>
server_connection::response_fields(const http_response_data& response)
{
    if (response.status < 100 || response.status > 599 ||
        ((response.status < 200 || response.status == 204 ||
          response.status == 304) && !response.body.empty()))
    {
        network::fail("invalid_argument", "HTTP/3 响应状态码或正文无效");
    }
    std::vector<std::pair<std::string, std::string>> result{
        {":status", std::to_string(response.status)}
    };
    std::size_t length = 0;
    for (const auto& [name, value] : response.headers)
    {
        network::validate_header(name, value);
        if (name != network::lower_ascii(name) ||
            name == "content-length" || name == "transfer-encoding" ||
            name == "connection" || name == "set-cookie")
        {
            network::fail("invalid_header", "HTTP/3 响应包含保留或大写头字段");
        }
        length += name.size() + value.size();
        result.emplace_back(name, value);
    }
    for (const auto& cookie : response.cookies)
    {
        network::validate_header("set-cookie", cookie);
        length += 10 + cookie.size();
        result.emplace_back("set-cookie", cookie);
    }
    auto size = std::to_string(response.body.size());
    length += 14 + size.size();
    if (length > network::max_head_bytes)
    {
        network::fail("size_limit", "HTTP/3 响应头超过 64 KiB");
    }
    result.emplace_back("content-length", std::move(size));
    return result;
}

void server_connection::respond(const std::shared_ptr<server_request>& request,
                                 const http_response_data& response)
{
    if (!request || response.body.size() > network::max_body_bytes)
    {
        network::fail("size_limit", "HTTP/3 响应正文超过 8 MiB");
    }
    auto fields = response_fields(response);
    std::vector<nghttp3_nv> values;
    values.reserve(fields.size());
    for (const auto& [name, value] : fields)
    {
        values.push_back({reinterpret_cast<const std::uint8_t*>(name.data()),
            reinterpret_cast<const std::uint8_t*>(value.data()),
            name.size(), value.size(), NGHTTP3_NV_FLAG_NONE});
    }
    {
        std::lock_guard lock(mutex_);
        if (!request->complete || request->response_started || shutdown_)
        {
            network::fail("connection_closed", "HTTP/3 请求流不可回复");
        }
        request->response_started = true;
        request->response_body = response.body;
        nghttp3_data_reader reader{read_body};
        if (nghttp3_conn_submit_response(h3_, request->stream_id,
                values.data(), values.size(), request->response_body.empty()
                    ? nullptr : &reader) != 0)
        {
            network::fail("protocol_error", "提交 HTTP/3 响应失败");
        }
        pump();
    }
    std::unique_lock lock(mutex_);
    const auto deadline = std::chrono::steady_clock::now() +
        std::chrono::seconds(30);
    if (!changed_.wait_until(lock, deadline, [&]
        {
            return request->response_done || shutdown_ || error_;
        }))
    {
        network::fail("timeout", "HTTP/3 响应发送超时");
    }
    check_error();
    if (!request->response_done)
    {
        network::fail("connection_closed", "HTTP/3 响应流提前关闭");
    }
    requests_.erase(request->stream_id);
}

void server_connection::close_stream(std::int64_t id) noexcept
{
    std::lock_guard lock(mutex_);
    const auto found = stream_handles_.find(id);
    if (found != stream_handles_.end())
    {
        api_->StreamShutdown(found->second,
            QUIC_STREAM_SHUTDOWN_FLAG_ABORT_SEND |
            QUIC_STREAM_SHUTDOWN_FLAG_ABORT_RECEIVE, 0);
    }
    requests_.erase(id);
}

} // namespace tx_generated::http3
