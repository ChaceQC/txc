#pragma once

#include "stdlib/httpx_client_session.hpp"
#include "stdlib/httpx_client_tls.hpp"

#include <curl/curl.h>
#include <chrono>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace tx_generated::httpx_curl
{
struct session
{
    std::mutex mutex;
    CURLM* multi = nullptr;
    CURLSH* share = nullptr;
    std::string proxy;
    bool decompress = false;
    bool allow_http2 = true;
    bool closed = false;
    std::shared_ptr<const httpx_client_tls::settings> tls;
    session(std::string_view proxy_url, std::int64_t max_connections,
        bool decode, bool http2);
    ~session();
};

enum class phase
{
    uploading,
    reading,
    eof
};

struct request
{
    std::shared_ptr<session> owner;
    CURL* easy = nullptr;
    curl_slist* headers = nullptr;
    bool attached = false;
    bool done = false;
    bool header_ready = false;
    bool require_http2 = false;
    bool receive_paused = false;
    bool send_paused = false;
    CURLcode result = CURLE_OK;
    phase current = phase::uploading;
    std::int64_t timeout_ms = 0;
    std::int64_t remaining_upload = 0;
    std::int64_t response_limit = 0;
    std::int64_t received = 0;
    std::string upload;
    std::size_t upload_offset = 0;
    std::string download;
    std::string raw_head;
    http_response_data response;
    std::exception_ptr failure;
    explicit request(std::shared_ptr<session> value);
    ~request();
    void detach() noexcept;
};

template<class value_type>
void option(CURL* easy, CURLoption key, value_type value)
{
    if (curl_easy_setopt(easy, key, value) != CURLE_OK)
    {
        network::fail("operation_failed", "配置 HTTP 传输失败");
    }
}

void configure(request& value, std::string_view method, std::string_view url,
    const network::header_map& headers, std::int64_t length);
void pump(request& value);
void resume(request& value);
void check(request& value);
void wait(request& value, const std::function<bool()>& ready);
}
