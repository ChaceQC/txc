#pragma once

#include "stdlib/httpx_client_tls.hpp"
#include "stdlib/httpx_client_session.hpp"

#include <memory>
#include <string_view>

namespace tx_generated::httpx_custom_tls
{

class session_pool
{
public:
    session_pool(std::shared_ptr<const httpx_client_tls::settings> tls,
        HINTERNET proxy_session, std::string_view proxy_url,
        std::int64_t max_connections, bool decompress, bool allow_http2);
    ~session_pool() noexcept;
    session_pool(const session_pool&) = delete;
    session_pool& operator=(const session_pool&) = delete;

    void close() noexcept;

private:
    struct state;
    std::shared_ptr<state> value_;
    friend class request;
};

class request
{
public:
    request(std::shared_ptr<session_pool> pool, std::string_view method,
            std::string_view url, const network::header_map& headers,
            std::int64_t body_length, std::int64_t max_response_bytes,
            std::int64_t timeout_ms, bool require_http2);
    ~request() noexcept;
    request(const request&) = delete;
    request& operator=(const request&) = delete;

    std::int64_t write(std::string_view data);
    http_response_data finish();
    httpx_session::response_chunk read(std::int64_t max_bytes);
    void close() noexcept;

private:
    struct state;
    std::unique_ptr<state> value_;
};

} // namespace tx_generated::httpx_custom_tls
