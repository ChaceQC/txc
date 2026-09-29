#include "stdlib/http3_server_internal.hpp"

#include "stdlib/error.hpp"

#include <algorithm>
#include <utility>

namespace tx_generated::http3
{

server_connection::server_connection(
    std::shared_ptr<server_listener> listener, HQUIC connection)
    : listener_(listener), api_(quic().get()),
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
    bool active = false;
    bool needs_shutdown = false;
    {
        std::lock_guard lock(mutex_);
        active = connection_ && !native_closed_;
        needs_shutdown = active && !shutdown_.load();
    }
    if (active)
    {
        if (needs_shutdown)
        {
            shutdown();
        }
        wait_native_shutdown();
    }
    close_native_handles();
}

void server_connection::close_native_handles() noexcept
{
    HQUIC connection = nullptr;
    nghttp3_conn* http3 = nullptr;
    {
        std::lock_guard lock(mutex_);
        // 流回调在锁外调用 StreamClose。只有所有回调返回后，才能销毁
        // 共享连接句柄和 nghttp3 对象，避免最终回调彼此并发时互相踩踏。
        if (native_closed_ || native_close_started_ ||
            !connection_shutdown_complete_ || !streams_.empty() ||
            connection_callbacks_active_ != 0 ||
            connection_operations_active_ != 0 ||
            stream_callbacks_active_ != 0 ||
            stream_closes_active_ != 0)
        {
            return;
        }
        native_close_started_ = true;
        stream_handles_.clear();
        connection = std::exchange(connection_, nullptr);
        http3 = std::exchange(h3_, nullptr);
    }
    if (connection)
    {
        api_->ConnectionClose(connection);
    }
    if (http3)
    {
        nghttp3_conn_del(http3);
    }
    {
        std::lock_guard lock(mutex_);
        native_close_started_ = false;
        native_closed_ = true;
        changed_.notify_all();
    }
}

void server_connection::reap_native_handles() noexcept
{
    close_native_handles();
}

void server_connection::wait_if_idle_closed() noexcept
{
    {
        std::lock_guard lock(mutex_);
        if (!listener_closed_ || !requests_.empty())
        {
            return;
        }
    }
    wait_native_shutdown();
}

void server_connection::wait_native_shutdown() noexcept
{
    while (true)
    {
        std::unique_lock lock(mutex_);
        changed_.wait(lock, [&]
        {
            return native_closed_.load() ||
                (connection_shutdown_complete_ && streams_.empty() &&
                 connection_callbacks_active_ == 0 &&
                 connection_operations_active_ == 0 &&
                 stream_callbacks_active_ == 0 &&
                 stream_closes_active_ == 0 && !native_close_started_);
        });
        if (native_closed_)
        {
            return;
        }
        lock.unlock();
        close_native_handles();
    }
}

void server_connection::reject_failed_configuration() noexcept
{
    // 监听回调返回失败时，连接句柄由 MsQuic 回收，避免再次 ConnectionClose。
    {
        std::lock_guard lock(mutex_);
        connection_ = nullptr;
        shutdown_ = true;
        connection_shutdown_complete_ = true;
    }
    close_native_handles();
}

void server_connection::shutdown() noexcept
{
    HQUIC connection = nullptr;
    {
        std::lock_guard lock(mutex_);
        if (!connection_ || shutdown_.load() ||
            shutdown_requested_.exchange(true))
        {
            return;
        }
        connection = connection_;
        ++connection_operations_active_;
    }
    if (connection)
    {
        api_->ConnectionShutdown(connection, QUIC_CONNECTION_SHUTDOWN_FLAG_NONE,
                                 0);
        std::lock_guard lock(mutex_);
        --connection_operations_active_;
        changed_.notify_all();
    }
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
    if (const auto listener = listener_.lock())
    {
        listener->report_error(current);
    }
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
        if (!request->complete || !request->delivered || request->closed ||
            request->response_started || shutdown_)
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
        return request->response_done || request->closed || shutdown_ || error_;
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
    const bool stop_idle_listener_connection = listener_closed_ &&
        requests_.empty();
    lock.unlock();
    if (stop_idle_listener_connection)
    {
        shutdown();
        wait_native_shutdown();
    }
}

void server_connection::close_stream(std::int64_t id) noexcept
{
    HQUIC handle = nullptr;
    stream_state* active_stream = nullptr;
    std::shared_ptr<server_request> retired;
    bool stop_idle_listener_connection = false;
    {
        std::lock_guard lock(mutex_);
        const auto found = stream_handles_.find(id);
        const auto stream = std::find_if(streams_.begin(), streams_.end(),
            [id](const auto& value)
            {
                return value->id == id;
            });
        if (stream != streams_.end() &&
            (*stream)->closure == stream_state::close_reason::active)
        {
            (*stream)->closure = stream_state::close_reason::local_abort;
        }
        if (stream != streams_.end() && !(*stream)->abort_requested &&
            !shutdown_ && !shutdown_requested_)
        {
            if (found != stream_handles_.end())
            {
                (*stream)->abort_requested = true;
                active_stream = stream->get();
                ++active_stream->native_operations_active;
                handle = found->second;
            }
        }
        const auto request = requests_.find(id);
        if (request != requests_.end())
        {
            request->second->closed = true;
            retired = std::move(request->second);
            requests_.erase(request);
        }
        stop_idle_listener_connection = listener_closed_ && requests_.empty();
        changed_.notify_all();
    }
    if (handle)
    {
        api_->StreamShutdown(handle,
            QUIC_STREAM_SHUTDOWN_FLAG_ABORT_SEND |
            QUIC_STREAM_SHUTDOWN_FLAG_ABORT_RECEIVE, 0);
        {
            std::lock_guard lock(mutex_);
            --active_stream->native_operations_active;
            changed_.notify_all();
        }
        reap_closed_streams();
    }
    if (stop_idle_listener_connection)
    {
        shutdown();
        wait_native_shutdown();
    }
}

bool server_connection::mark_delivered(
    const std::shared_ptr<server_request>& request) noexcept
{
    if (!request)
    {
        return false;
    }
    std::lock_guard lock(mutex_);
    const auto found = requests_.find(request->stream_id);
    if (listener_closed_ || shutdown_ || request->closed ||
        found == requests_.end() || found->second != request)
    {
        return false;
    }
    request->delivered = true;
    return true;
}

void server_connection::close_unclaimed_requests() noexcept
{
    std::vector<std::int64_t> streams;
    bool stop_idle_listener_connection = false;
    {
        std::lock_guard lock(mutex_);
        listener_closed_ = true;
        try
        {
            streams.reserve(requests_.size());
        }
        catch (...)
        {
            changed_.notify_all();
            return;
        }
        for (auto item = requests_.begin(); item != requests_.end();)
        {
            if (!item->second->delivered)
            {
                item->second->closed = true;
                streams.push_back(item->first);
                item = requests_.erase(item);
            }
            else
            {
                ++item;
            }
        }
        stop_idle_listener_connection = requests_.empty();
        changed_.notify_all();
    }
    for (const auto id : streams)
    {
        close_stream(id);
    }
    if (stop_idle_listener_connection)
    {
        shutdown();
        wait_native_shutdown();
    }
}

} // namespace tx_generated::http3
