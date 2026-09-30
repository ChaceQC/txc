#include "stdlib/httpx_curl.hpp"

#include <algorithm>
#include <cstring>
#include <limits>

namespace tx_generated::httpx_curl
{
namespace
{
std::size_t upload_data(char* buffer, std::size_t size, std::size_t count, void* user) noexcept
{
    auto& value = *static_cast<request*>(user);
    const auto available = value.upload.size() - value.upload_offset;
    if (available == 0)
    {
        if (value.remaining_upload == 0)
        {
            return 0;
        }
        value.send_paused = true;
        return CURL_READFUNC_PAUSE;
    }
    const auto amount = std::min(available, size * count);
    std::memcpy(buffer, value.upload.data() + value.upload_offset, amount);
    value.upload_offset += amount;
    return amount;
}

std::size_t download_data(char* buffer, std::size_t size, std::size_t count, void* user) noexcept
{
    auto& value = *static_cast<request*>(user);
    const auto amount = size * count;
    if (value.current == phase::uploading || !value.download.empty())
    {
        value.receive_paused = true;
        return CURL_WRITEFUNC_PAUSE;
    }
    try
    {
        if (amount > static_cast<std::uint64_t>(value.response_limit - value.received))
        {
            network::fail("size_limit", "HTTP 响应正文超过接收上限");
        }
        value.download.assign(buffer, amount);
        value.received += static_cast<std::int64_t>(amount);
        return amount;
    }
    catch (...)
    {
        value.failure = std::current_exception();
        return 0;
    }
}

std::size_t header_data(char* buffer, std::size_t size, std::size_t count, void* user) noexcept
{
    auto& value = *static_cast<request*>(user);
    const std::string_view line(buffer, size * count);
    try
    {
        if (line.starts_with("HTTP/"))
        {
            value.raw_head.clear();
            value.header_ready = false;
        }
        if (value.header_ready)
        {
            // trailers 仍接受字段验证，但不混入最终响应头。
            if (line != "\r\n")
            {
                const auto colon = line.find(':');
                if (colon == std::string_view::npos)
                {
                    network::fail("protocol_error", "HTTP trailer 格式无效");
                }
                network::validate_header(line.substr(0, colon),
                    line.substr(colon + 1, line.size() - colon - 3));
            }
            return line.size();
        }
        if (value.raw_head.size() + line.size() > network::max_head_bytes)
        {
            network::fail("size_limit", "HTTP 响应头超过 64 KiB");
        }
        value.raw_head += line;
        if (line == "\r\n")
        {
            auto head = network::parse_head(value.raw_head);
            const auto space = head.first_line.find(' ');
            const auto status = head.first_line.substr(space + 1, 3);
            if (status.size() != 3 || status.find_first_not_of("0123456789") != std::string::npos)
            {
                network::fail("protocol_error", "HTTP 响应状态码无效");
            }
            value.response.status = std::stoi(status);
            if (value.response.status >= 200)
            {
                value.response.headers = std::move(head.headers);
                value.response.cookies = std::move(head.cookies);
                value.header_ready = true;
                if (!value.owner->decompress)
                {
                    (void)network::content_length(value.response.headers, value.response_limit);
                }
            }
            else
            {
                value.raw_head.clear();
            }
        }
        return line.size();
    }
    catch (...)
    {
        value.failure = std::current_exception();
        return 0;
    }
}
}

session::session(std::string_view proxy_url, std::int64_t max_connections,
    bool decode, bool http2) : proxy(proxy_url), decompress(decode), allow_http2(http2)
{
    static const auto initialized = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (initialized != CURLE_OK)
    {
        network::fail("operation_failed", "初始化 HTTP 客户端失败");
    }
    multi = curl_multi_init();
    share = curl_share_init();
    if (!multi || !share)
    {
        if (multi)
        {
            curl_multi_cleanup(multi);
        }
        if (share)
        {
            curl_share_cleanup(share);
        }
        network::fail("operation_failed", "创建 HTTP 会话失败");
    }
    curl_multi_setopt(multi, CURLMOPT_MAX_HOST_CONNECTIONS, static_cast<long>(max_connections));
    // 同一会话的所有 CURL 调用由 mutex 串行化，共享 Cookie 不需要额外锁。
    curl_share_setopt(share, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
}

session::~session()
{
    curl_multi_cleanup(multi);
    curl_share_cleanup(share);
}

request::request(std::shared_ptr<session> value) : owner(std::move(value)), easy(curl_easy_init())
{
    if (!easy)
    {
        network::fail("operation_failed", "创建 HTTP 请求失败");
    }
}

void request::detach() noexcept
{
    if (attached)
    {
        curl_multi_remove_handle(owner->multi, easy);
        attached = false;
    }
}

request::~request()
{
    detach();
    curl_easy_cleanup(easy);
    curl_slist_free_all(headers);
}

void configure(request& value, std::string_view method, std::string_view url,
    const network::header_map& fields, std::int64_t length)
{
    network::validate_token(method, "HTTP 方法");
    const auto address = network::parse_url(url, false);
    auto* easy = value.easy;
    option(easy, CURLOPT_URL, std::string(url).c_str());
    option(easy, CURLOPT_CUSTOMREQUEST, std::string(method).c_str());
    option(easy, CURLOPT_PATH_AS_IS, 1L);
    option(easy, CURLOPT_NOSIGNAL, 1L);
    option(easy, CURLOPT_FOLLOWLOCATION, 0L);
    option(easy, CURLOPT_CONNECTTIMEOUT_MS, static_cast<long>(value.timeout_ms));
    option(easy, CURLOPT_USERAGENT, "TX/1.0");
    option(easy, CURLOPT_SHARE, value.owner->share);
    option(easy, CURLOPT_COOKIEFILE, "");
    option(easy, CURLOPT_SUPPRESS_CONNECT_HEADERS, 1L);
    option(easy, CURLOPT_PROTOCOLS_STR, "http,https");
    option(easy, CURLOPT_SSL_VERIFYPEER, 1L);
    option(easy, CURLOPT_SSL_VERIFYHOST, 2L);
    option(easy, CURLOPT_HTTP_VERSION, value.require_http2
        ? static_cast<long>(address.secure ? CURL_HTTP_VERSION_2TLS : CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE)
        : static_cast<long>(value.owner->allow_http2 ? CURL_HTTP_VERSION_2TLS : CURL_HTTP_VERSION_1_1));
    if (value.owner->proxy == "direct")
    {
        option(easy, CURLOPT_PROXY, "");
    }
    else if (!value.owner->proxy.empty())
    {
        option(easy, CURLOPT_PROXY, value.owner->proxy.c_str());
        option(easy, CURLOPT_NOPROXY, "");
    }
    if (value.owner->decompress)
    {
        option(easy, CURLOPT_ACCEPT_ENCODING, "gzip, deflate");
    }
    if (value.owner->tls)
    {
        httpx_client_tls::configure(easy, *value.owner->tls);
    }
    const auto append = [&](const std::string& text)
    {
        auto* updated = curl_slist_append(value.headers, text.c_str());
        if (!updated)
        {
            network::fail("operation_failed", "分配 HTTP 请求头失败");
        }
        value.headers = updated;
    };
    append("Expect:");
    std::size_t header_bytes = 0;
    for (const auto& [name, content] : fields)
    {
        network::validate_header(name, content);
        const auto lower = network::lower_ascii(name);
        if (lower == "content-length" || lower == "transfer-encoding" ||
            lower == "host" || lower == "connection" || lower == "proxy-connection" ||
            lower == "keep-alive" || lower == "upgrade")
        {
            network::fail("invalid_header", "请求正文长度由 HTTP 客户端管理");
        }
        append(name + (content.empty() ? ";" : ": " + content));
        header_bytes += name.size() + content.size() + 4;
        if (header_bytes > network::max_head_bytes)
        {
            network::fail("size_limit", "HTTP 请求头超过 64 KiB");
        }
    }
    option(easy, CURLOPT_HTTPHEADER, value.headers);
    option(easy, CURLOPT_HEADERFUNCTION, header_data);
    option(easy, CURLOPT_HEADERDATA, &value);
    option(easy, CURLOPT_WRITEFUNCTION, download_data);
    option(easy, CURLOPT_WRITEDATA, &value);
    option(easy, CURLOPT_READFUNCTION, upload_data);
    option(easy, CURLOPT_READDATA, &value);
    option(easy, CURLOPT_PRIVATE, &value);
    if (length > 0 || method == "POST" || method == "PUT" || method == "PATCH")
    {
        option(easy, CURLOPT_UPLOAD, 1L);
        option(easy, CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(length));
    }
    if (method == "HEAD")
    {
        option(easy, CURLOPT_NOBODY, 1L);
    }
    if (curl_multi_add_handle(value.owner->multi, easy) != CURLM_OK)
    {
        network::fail("operation_failed", "启动 HTTP 请求失败");
    }
    value.attached = true;
}

void check(request& value)
{
    if (value.owner->closed)
    {
        network::fail("connection_closed", "HTTP 会话已关闭");
    }
    if (value.failure)
    {
        std::rethrow_exception(value.failure);
    }
    if (value.result != CURLE_OK)
    {
        const char* code = value.result == CURLE_OPERATION_TIMEDOUT ? "timeout" :
            value.result == CURLE_PEER_FAILED_VERIFICATION ||
            value.result == CURLE_SSL_CERTPROBLEM ||
            value.result == CURLE_SSL_CACERT_BADFILE ? "security_error" :
            value.result == CURLE_HTTP2 || value.result == CURLE_HTTP2_STREAM ||
            value.result == CURLE_PARTIAL_FILE ? "protocol_error" : "operation_failed";
        network::fail(code, std::string("HTTP 传输失败：") + curl_easy_strerror(value.result));
    }
    if (value.owner->decompress && value.received > 1024)
    {
        curl_off_t encoded = 0;
        curl_easy_getinfo(value.easy, CURLINFO_SIZE_DOWNLOAD_T, &encoded);
        if (encoded > 0 && static_cast<std::uint64_t>(value.received) >
            static_cast<std::uint64_t>(encoded) * 100 + 1024)
        {
            value.detach();
            network::fail("size_limit", "HTTP 响应超过 100 倍解压比例上限");
        }
    }
}

void pump(request& value)
{
    int running = 0;
    if (curl_multi_perform(value.owner->multi, &running) != CURLM_OK)
    {
        network::fail("operation_failed", "执行 HTTP 传输失败");
    }
    int remaining = 0;
    while (auto* message = curl_multi_info_read(value.owner->multi, &remaining))
    {
        if (message->msg == CURLMSG_DONE)
        {
            request* completed = nullptr;
            curl_easy_getinfo(message->easy_handle, CURLINFO_PRIVATE, &completed);
            completed->done = true;
            completed->result = message->data.result;
            completed->detach();
        }
    }
    check(value);
}

void resume(request& value)
{
    if (!value.done && (value.send_paused || value.receive_paused))
    {
        value.send_paused = value.upload_offset == value.upload.size() && value.remaining_upload != 0;
        value.receive_paused = value.receive_paused &&
            (value.current == phase::uploading || !value.download.empty());
        const int flags = (value.send_paused ? CURLPAUSE_SEND : 0) |
            (value.receive_paused ? CURLPAUSE_RECV : 0);
        const auto result = curl_easy_pause(value.easy, flags);
        if (result != CURLE_OK)
        {
            value.result = result;
        }
        check(value);
    }
}

void wait(request& value, const std::function<bool()>& ready)
{
    const auto deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(value.timeout_ms);
    while (!ready())
    {
        pump(value);
        if (ready())
        {
            return;
        }
        if (value.done)
        {
            network::fail("protocol_error", "HTTP 对端在完成请求前结束传输");
        }
        if (std::chrono::steady_clock::now() >= deadline)
        {
            value.detach();
            value.result = CURLE_OPERATION_TIMEDOUT;
            check(value);
        }
        int descriptors = 0;
        if (curl_multi_poll(value.owner->multi, nullptr, 0, 10, &descriptors) != CURLM_OK)
        {
            network::fail("operation_failed", "等待 HTTP 网络事件失败");
        }
    }
    check(value);
}
}
