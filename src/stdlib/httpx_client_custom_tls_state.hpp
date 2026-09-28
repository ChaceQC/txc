#pragma once

#include "stdlib/httpx_client_custom_tls.hpp"
#include "stdlib/httpx_client_custom_tls_transport.hpp"
#include "stdlib/http2_client.hpp"
#include "stdlib/tls_stream.hpp"
#include "stdlib/error.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace tx_generated::httpx_custom_tls
{
using clock = std::chrono::steady_clock;

struct session_pool::state
{
    struct connection
    {
        std::shared_ptr<tls::secure_connection> tls;
        std::shared_ptr<http2::client_session> http2;
        bool uses_http2 = false;
    };

    struct lease
    {
        connection value;
        bool reserved_new = false;
    };

    std::shared_ptr<const httpx_client_tls::settings> tls_settings;
    HINTERNET proxy_session = nullptr;
    std::string proxy_url;
    std::int64_t max_connections = 0;
    bool decompress = false;
    bool allow_http2 = false;
    std::mutex mutex;
    std::condition_variable changed;
    bool closed = false;
    std::unordered_map<std::string, std::vector<connection>> idle;
    std::unordered_map<std::string, std::size_t> opened;

    static void close_connection(connection value) noexcept
    {
        value.http2.reset();
        if (value.tls)
        {
            try
            {
                value.tls->close(1000);
            }
            catch (...)
            {
            }
        }
    }

    lease acquire(std::string_view key, bool require_http2,
                  std::int64_t timeout_ms)
    {
        const auto deadline = clock::now() +
            std::chrono::milliseconds(timeout_ms);
        std::unique_lock lock(mutex);
        while (true)
        {
            if (closed)
            {
                network::fail("connection_closed", "HTTP TLS 会话已关闭");
            }
            const std::string pool_key(key);
            auto& available = idle[pool_key];
            const auto found = std::find_if(available.begin(), available.end(),
                [require_http2](const connection& item)
                {
                    return !require_http2 || item.uses_http2;
                });
            if (found != available.end())
            {
                lease result{std::move(*found), false};
                available.erase(found);
                return result;
            }
            if (opened[pool_key] < static_cast<std::size_t>(max_connections))
            {
                ++opened[pool_key];
                return {{}, true};
            }
            if (!available.empty())
            {
                connection discarded = std::move(available.back());
                available.pop_back();
                if (opened[pool_key] > 0)
                {
                    --opened[pool_key];
                }
                lock.unlock();
                close_connection(std::move(discarded));
                lock.lock();
                continue;
            }
            if (changed.wait_until(lock, deadline) == std::cv_status::timeout)
            {
                network::fail("timeout", "等待可用 HTTP TLS 连接超时");
            }
        }
    }

    void abandon(std::string_view key) noexcept
    {
        {
            std::lock_guard lock(mutex);
            auto& count = opened[std::string(key)];
            if (count > 0)
            {
                --count;
            }
        }
        changed.notify_one();
    }

    void release(std::string_view key, connection value, bool reusable) noexcept
    {
        bool discard = false;
        try
        {
            std::lock_guard lock(mutex);
            auto& count = opened[std::string(key)];
            if (closed || !reusable || !value.tls || value.tls->closed())
            {
                if (count > 0)
                {
                    --count;
                }
                discard = true;
            }
            else
            {
                try
                {
                    idle[std::string(key)].push_back(std::move(value));
                }
                catch (...)
                {
                    if (count > 0)
                    {
                        --count;
                    }
                    discard = true;
                }
            }
        }
        catch (...)
        {
            discard = true;
        }
        changed.notify_one();
        if (discard)
        {
            close_connection(std::move(value));
        }
    }

    void close() noexcept
    {
        std::vector<connection> discarded;
        {
            std::lock_guard lock(mutex);
            if (closed)
            {
                return;
            }
            closed = true;
            for (auto& [key, items] : idle)
            {
                auto& count = opened[key];
                count = count > items.size() ? count - items.size() : 0;
                for (auto& item : items)
                {
                    discarded.push_back(std::move(item));
                }
            }
            idle.clear();
        }
        changed.notify_all();
        for (auto& item : discarded)
        {
            close_connection(std::move(item));
        }
    }
};

struct request::state
{
    enum class phase
    {
        uploading,
        reading,
        eof
    };

    std::shared_ptr<session_pool> pool;
    std::string pool_key;
    std::shared_ptr<tls::secure_connection> connection;
    std::shared_ptr<http2::client_session> http2;
    std::string pending;
    std::string method;
    std::string target;
    std::string target_authority;
    network::header_map headers;
    std::int64_t timeout_ms = 0;
    std::int64_t upload_length = 0;
    std::uint64_t upload_remaining = 0;
    std::uint64_t response_limit = 0;
    std::uint64_t response_received = 0;
    std::uint64_t content_remaining = 0;
    std::uint64_t chunk_remaining = 0;
    bool has_content_length = false;
    bool chunked = false;
    bool body_allowed = true;
    bool close_delimited = false;
    bool chunked_end = false;
    bool uses_http2 = false;
    bool response_reusable = false;
    bool lease_active = false;
    bool decompress = false;
    phase current = phase::uploading;
    http_response_data response;

    ~state() noexcept
    {
        close_connection();
    }

    std::string receive(std::size_t amount)
    {
        socket::read_result result;
        try
        {
            result = connection->read(static_cast<std::int64_t>(amount),
                                      timeout_ms);
        }
        catch (const runtime_failure& failure)
        {
            if (failure.error().code == "truncated_close")
            {
                network::fail("connection_closed",
                    "HTTP TLS 对端在响应完成前关闭了连接");
            }
            throw;
        }
        if (result.eof)
        {
            return {};
        }
        return {reinterpret_cast<const char*>(result.data.data()),
                result.data.size()};
    }

    std::string read_exact(std::size_t amount)
    {
        std::string result;
        result.reserve(amount);
        const auto buffered = std::min(amount, pending.size());
        result.append(pending.data(), buffered);
        pending.erase(0, buffered);
        while (result.size() < amount)
        {
            auto block = receive(std::min<std::size_t>(
                amount - result.size(), 16 * 1024));
            if (block.empty())
            {
                network::fail("connection_closed", "HTTP 响应正文提前结束");
            }
            result += block;
        }
        return result;
    }

    std::string read_line(std::size_t limit)
    {
        while (true)
        {
            const auto end = pending.find("\r\n");
            if (end != std::string::npos)
            {
                if (end > limit)
                {
                    network::fail("size_limit", "HTTP 分块行超过限制");
                }
                auto result = pending.substr(0, end);
                pending.erase(0, end + 2);
                return result;
            }
            if (pending.size() >= limit)
            {
                network::fail("size_limit", "HTTP 分块行超过限制");
            }
            auto block = receive(4096);
            if (block.empty())
            {
                network::fail("connection_closed", "HTTP 响应行提前结束");
            }
            pending += block;
        }
    }

    std::string read_head()
    {
        while (true)
        {
            const auto end = pending.find("\r\n\r\n");
            if (end != std::string::npos)
            {
                auto result = pending.substr(0, end + 4);
                pending.erase(0, end + 4);
                return result;
            }
            if (pending.size() >= network::max_head_bytes)
            {
                network::fail("size_limit", "HTTP 响应头超过 64 KiB");
            }
            auto block = receive(4096);
            if (block.empty())
            {
                network::fail("connection_closed", "HTTP 响应头提前结束");
            }
            pending += block;
        }
    }

    void release_connection(bool reusable) noexcept
    {
        session_pool::state::connection pooled{
            std::move(connection), std::move(http2), uses_http2};
        if (!lease_active || !pool || !pool->value_)
        {
            session_pool::state::close_connection(std::move(pooled));
            lease_active = false;
            return;
        }
        pool->value_->release(pool_key, std::move(pooled),
            reusable && pending.empty());
        lease_active = false;
    }

    void close_connection() noexcept
    {
        release_connection(false);
    }

    void read_response_head()
    {
        while (true)
        {
            auto parsed = network::parse_head(read_head());
            validate_status_line(parsed);
            const std::string_view line(parsed.first_line);
            const auto first_space = line.find(' ');
            int status = 0;
            const auto code = line.substr(first_space + 1, 3);
            const auto [end, error] = std::from_chars(code.data(),
                code.data() + code.size(), status);
            if (error != std::errc{} || end != code.data() + code.size() ||
                status < 100 || status > 599)
            {
                network::fail("protocol_error", "HTTP 响应状态码无效");
            }
            if (status < 200)
            {
                if (status == 101)
                {
                    network::fail("protocol_error", "HTTP TLS 客户端不接受协议升级");
                }
                continue;
            }
            response.status = status;
            response.protocol = line.starts_with("HTTP/1.0 ")
                ? "http/1.0" : "http/1.1";
            response.headers = std::move(parsed.headers);
            response.cookies = std::move(parsed.cookies);
            break;
        }
        body_allowed = method != "HEAD" && response.status != 204 &&
            response.status != 304;
        const auto content_encoding = response.headers.find("content-encoding");
        if (decompress && content_encoding != response.headers.end() &&
            network::lower_ascii(content_encoding->second) != "identity")
        {
            network::fail("protocol_error",
                "自定义 CA TLS 路径未能解码压缩响应");
        }
        const bool is_http_11 = response.protocol == "http/1.1";
        const bool explicit_keep_alive = response_header_has_token(response.headers,
            "connection", "keep-alive");
        response_reusable = !response_header_has_token(response.headers,
            "connection", "close") && (is_http_11 || explicit_keep_alive);
        const auto transfer = response.headers.find("transfer-encoding");
        const auto length = response.headers.find("content-length");
        if (transfer != response.headers.end())
        {
            if (length != response.headers.end() ||
                network::lower_ascii(transfer->second) != "chunked")
            {
                network::fail("protocol_error", "HTTP 响应传输编码不受支持");
            }
            chunked = true;
        }
        else if (length != response.headers.end())
        {
            content_remaining = network::content_length(response.headers,
                static_cast<std::size_t>(response_limit));
            has_content_length = true;
        }
        else
        {
            close_delimited = body_allowed;
            if (close_delimited)
            {
                response_reusable = false;
            }
        }
        current = phase::reading;
    }

    void finish_chunked()
    {
        while (true)
        {
            const auto trailer = read_line(network::max_head_bytes);
            if (trailer.empty())
            {
                chunked_end = true;
                current = phase::eof;
                release_connection(response_reusable);
                return;
            }
            const auto colon = trailer.find(':');
            if (colon == std::string::npos)
            {
                network::fail("protocol_error", "HTTP 分块尾字段无效");
            }
            network::validate_header(std::string_view(trailer).substr(0, colon),
                std::string_view(trailer).substr(colon + 1));
        }
    }
};

} // namespace tx_generated::httpx_custom_tls
