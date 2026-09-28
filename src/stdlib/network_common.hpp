#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <windows.h>
#include <winhttp.h>

#include <cstdint>
#include <chrono>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tx_generated::tls
{
class secure_connection;
}

namespace tx_generated::network
{

constexpr std::size_t max_head_bytes = 64 * 1024;
constexpr std::size_t max_body_bytes = 8 * 1024 * 1024;
using header_map = std::map<std::string, std::string>;

struct parsed_head
{
    std::string first_line;
    header_map headers;
    std::vector<std::string> cookies;
};

struct parsed_url
{
    std::wstring host;
    std::wstring target;
    INTERNET_PORT port = 0;
    bool secure = false;
};

[[noreturn]] void fail(std::string code, std::string message);
void validate_utf8(std::string_view text);
void validate_token(std::string_view text, std::string_view description);
void validate_header(std::string_view name, std::string_view value);
std::string lower_ascii(std::string_view text);
parsed_head parse_head(std::string_view text);
std::size_t content_length(const header_map& headers,
                           std::size_t max_length = max_body_bytes);
parsed_url parse_url(std::string_view text, bool websocket);
std::string base64(std::string_view bytes);
std::string sha1(std::string_view bytes);

class socket_handle
{
public:
    explicit socket_handle(SOCKET value = INVALID_SOCKET) noexcept : value_(value)
    {
    }
    socket_handle(const socket_handle&) = delete;
    socket_handle& operator=(const socket_handle&) = delete;
    socket_handle(socket_handle&& other) noexcept;
    socket_handle& operator=(socket_handle&& other) noexcept;
    ~socket_handle();
    [[nodiscard]] SOCKET get() const noexcept
    {
        return value_;
    }
    [[nodiscard]] bool valid() const noexcept
    {
        return value_ != INVALID_SOCKET;
    }
    void reset(SOCKET value = INVALID_SOCKET) noexcept;
private:
    SOCKET value_;
};

class http_handle
{
public:
    explicit http_handle(HINTERNET value = nullptr) noexcept : value_(value)
    {
    }
    http_handle(const http_handle&) = delete;
    http_handle& operator=(const http_handle&) = delete;
    http_handle(http_handle&& other) noexcept;
    http_handle& operator=(http_handle&& other) noexcept;
    ~http_handle();
    [[nodiscard]] HINTERNET get() const noexcept
    {
        return value_;
    }
    void reset(HINTERNET value = nullptr) noexcept;
private:
    HINTERNET value_;
};

class tcp_stream
{
public:
    explicit tcp_stream(socket_handle socket) : socket_(std::move(socket))
    {
    }
    explicit tcp_stream(std::shared_ptr<tls::secure_connection> secure)
        : secure_(std::move(secure))
    {
    }
    [[nodiscard]] SOCKET socket() const noexcept
    {
        return socket_.get();
    }
    [[nodiscard]] bool valid() const noexcept;
    void set_receive_timeout(std::int64_t timeout_ms);
    void set_receive_deadline(std::int64_t timeout_ms);
    void clear_receive_deadline();
    [[nodiscard]] std::string read_head();
    [[nodiscard]] std::string read_exact(std::size_t length);
    void send_all(std::string_view data);
    void close() noexcept;
private:
    [[nodiscard]] std::string read_some(std::size_t limit);
    std::size_t send_some(std::string_view data);
    socket_handle socket_;
    std::shared_ptr<tls::secure_connection> secure_;
    std::int64_t receive_timeout_ms_ = 0;
    std::optional<std::chrono::steady_clock::time_point> receive_deadline_;
    std::string pending_;
};

socket_handle listen_tcp(std::string_view host, std::int64_t port);
socket_handle accept_tcp(SOCKET listener, std::int64_t timeout_ms);
void initialize_winsock();
[[noreturn]] void socket_failure(std::string_view action);
[[noreturn]] void http_failure(std::string_view action);

} // namespace tx_generated::network
