#include "stdlib/httpx_client_custom_tls_state.hpp"

#include "stdlib/error.hpp"

#include <algorithm>
#include <charconv>
#include <utility>

namespace tx_generated::httpx_custom_tls
{

httpx_session::response_chunk request::read(std::int64_t max_bytes)
{
    if (max_bytes < 1 || max_bytes > 16 * 1024)
    {
        network::fail("invalid_argument", "HTTP 读取块须为 1～16384 字节");
    }
    if (value_->current == state::phase::uploading)
    {
        network::fail("invalid_argument", "HTTP 请求尚未读取响应头");
    }
    if (value_->current == state::phase::eof || !value_->body_allowed)
    {
        value_->current = state::phase::eof;
        value_->release_connection(value_->response_reusable);
        return {{}, true};
    }
    if (value_->uses_http2)
    {
        auto result = value_->http2->read_response_chunk(
            static_cast<std::size_t>(max_bytes));
        if (result.eof)
        {
            value_->current = state::phase::eof;
            value_->release_connection(true);
        }
        return {std::move(result.data), result.eof};
    }
    if (value_->chunked)
    {
        if (value_->chunk_remaining == 0)
        {
            const auto line = value_->read_line(8192);
            const auto extension = line.find(';');
            const auto size_text = std::string_view(line).substr(0, extension);
            std::uint64_t size = 0;
            const auto [end, error] = std::from_chars(size_text.data(),
                size_text.data() + size_text.size(), size, 16);
            if (error != std::errc{} || end != size_text.data() + size_text.size())
            {
                network::fail("protocol_error", "HTTP 分块长度无效");
            }
            if (size == 0)
            {
                value_->finish_chunked();
                return {{}, true};
            }
            value_->chunk_remaining = size;
        }
        const auto allowed = value_->response_limit - value_->response_received;
        if (value_->chunk_remaining > allowed)
        {
            network::fail("size_limit", "HTTP 响应正文超过大小上限");
        }
        const auto amount = static_cast<std::size_t>(std::min<std::uint64_t>(
            std::min<std::int64_t>(max_bytes, 16 * 1024),
            value_->chunk_remaining));
        auto result = value_->read_exact(amount);
        value_->chunk_remaining -= amount;
        value_->response_received += amount;
        if (value_->chunk_remaining == 0 && value_->read_exact(2) != "\r\n")
        {
            network::fail("protocol_error", "HTTP 分块正文缺少结束符");
        }
        return {std::move(result), false};
    }
    if (value_->has_content_length)
    {
        if (value_->content_remaining == 0)
        {
            value_->current = state::phase::eof;
            value_->release_connection(value_->response_reusable);
            return {{}, true};
        }
        const auto amount = static_cast<std::size_t>(std::min<std::uint64_t>(
            std::min<std::int64_t>(max_bytes, 16 * 1024),
            value_->content_remaining));
        auto result = value_->read_exact(amount);
        value_->content_remaining -= amount;
        value_->response_received += amount;
        return {std::move(result), false};
    }
    if (value_->close_delimited)
    {
        auto result = value_->receive(static_cast<std::size_t>(max_bytes));
        if (result.empty())
        {
            value_->current = state::phase::eof;
            value_->release_connection(false);
            return {{}, true};
        }
        if (result.size() > value_->response_limit - value_->response_received)
        {
            network::fail("size_limit", "HTTP 响应正文超过大小上限");
        }
        value_->response_received += result.size();
        return {std::move(result), false};
    }
    value_->current = state::phase::eof;
    value_->release_connection(value_->response_reusable);
    return {{}, true};
}

} // namespace tx_generated::httpx_custom_tls
