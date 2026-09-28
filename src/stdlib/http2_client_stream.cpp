#include "stdlib/http2_client.hpp"

#include "stdlib/error.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace tx_generated::http2
{

void client_session::start(std::string_view method, std::string_view scheme,
    std::string_view target, std::string_view authority,
    const network::header_map& headers, std::int64_t body_length,
    std::int64_t max_response_bytes)
{
    if (body_length < 0 || max_response_bytes < 0)
    {
        network::fail("invalid_argument", "HTTP/2 请求长度或接收上限无效");
    }
    response_ = {};
    destination_ = {};
    source_ = {};
    body_ = {};
    request_offset_ = 0;
    header_bytes_ = 0;
    stream_id_ = 0;
    complete_ = false;
    error_ = nullptr;
    response_headers_ready_ = false;
    response_offset_ = 0;
    upload_buffer_.clear();
    upload_offset_ = 0;
    streaming_input_ = true;
    upload_finished_ = false;
    upload_unsubmitted_remaining_ = static_cast<std::uint64_t>(body_length);
    request_remaining_ = upload_unsubmitted_remaining_;
    max_response_bytes_ = max_response_bytes;
    binary_ = true;
    submit_request(method, scheme, target, authority, headers, body_length);
    flush_output();
}

void client_session::write_body(std::string_view data)
{
    if (!streaming_input_ || upload_finished_)
    {
        network::fail("invalid_state", "HTTP/2 请求上传已结束");
    }
    if (data.size() > upload_unsubmitted_remaining_)
    {
        network::fail("size_limit", "HTTP/2 请求正文超过声明长度");
    }
    upload_unsubmitted_remaining_ -= data.size();
    upload_buffer_.append(data);
    while (upload_offset_ < upload_buffer_.size())
    {
        if (complete_)
        {
            network::fail("connection_closed", "HTTP/2 对端在上传完成前结束请求");
        }
        if (nghttp2_session_resume_data(session_, stream_id_) != 0)
        {
            network::fail("operation_failed", "恢复 HTTP/2 请求正文失败");
        }
        const auto before = upload_offset_;
        flush_output();
        if (upload_offset_ == before)
        {
            read_input();
        }
    }
}

void client_session::finish_upload()
{
    if (!streaming_input_ || upload_finished_ ||
        upload_unsubmitted_remaining_ != 0 ||
        upload_offset_ < upload_buffer_.size())
    {
        network::fail("invalid_argument", "HTTP/2 请求正文未完整写入或已结束");
    }
    upload_finished_ = true;
    if (request_remaining_ > 0 &&
        nghttp2_session_resume_data(session_, stream_id_) != 0)
    {
        network::fail("operation_failed", "完成 HTTP/2 请求上传失败");
    }
    flush_output();
}

http_response_data client_session::response_headers()
{
    while (!response_headers_ready_ && !complete_)
    {
        flush_output();
        if (!response_headers_ready_ && !complete_)
        {
            read_input();
        }
    }
    if (response_.status == 0)
    {
        network::fail("protocol_error", "HTTP/2 响应缺少状态码");
    }
    auto result = response_;
    result.protocol = "h2";
    result.body.clear();
    return result;
}

client_session::response_chunk client_session::read_response_chunk(
    std::size_t max_bytes)
{
    if (!streaming_input_ || !response_headers_ready_ || max_bytes == 0 ||
        max_bytes > 16 * 1024)
    {
        network::fail("invalid_argument", "HTTP/2 响应读取状态或长度无效");
    }
    while (response_offset_ == response_.body.size() && !complete_)
    {
        flush_output();
        if (response_offset_ == response_.body.size() && !complete_)
        {
            read_input();
        }
    }
    if (response_offset_ == response_.body.size())
    {
        if (response_.headers.contains("content-length") &&
            network::content_length(response_.headers,
                static_cast<std::size_t>(max_response_bytes_)) !=
            static_cast<std::size_t>(response_.body_length))
        {
            network::fail("protocol_error", "HTTP/2 响应长度与正文不符");
        }
        return {{}, true};
    }
    const auto amount = std::min(max_bytes,
        response_.body.size() - response_offset_);
    auto result = response_.body.substr(response_offset_, amount);
    response_offset_ += amount;
    return {std::move(result), false};
}

} // namespace tx_generated::http2
