#include "stdlib/network_common.hpp"

#include "stdlib/error.hpp"
#include "stdlib/tls_stream.hpp"

#include <algorithm>
#include <charconv>
#include <limits>
#include <stdexcept>
#include <utility>

namespace tx_generated::network
{
namespace
{

std::string trim_ows(std::string_view text)
{
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t'))
    {
        text.remove_prefix(1);
    }
    while (!text.empty() && (text.back() == ' ' || text.back() == '\t'))
    {
        text.remove_suffix(1);
    }
    return std::string(text);
}

} // namespace

[[noreturn]] void fail(std::string code, std::string message)
{
    throw runtime_failure({tx::error_kind::io, std::move(code), std::move(message)});
}

void validate_utf8(std::string_view text)
{
    for (std::size_t index = 0; index < text.size();)
    {
        const auto first = static_cast<unsigned char>(text[index]);
        if (first < 0x80)
        {
            ++index;
            continue;
        }
        const std::size_t count = first >= 0xc2 && first <= 0xdf ? 2 :
            first >= 0xe0 && first <= 0xef ? 3 :
            first >= 0xf0 && first <= 0xf4 ? 4 : 0;
        if (count == 0 || index + count > text.size())
        {
            fail("invalid_utf8", "文本包含无效的 UTF-8 字节序列");
        }
        for (std::size_t part = 1; part < count; ++part)
        {
            if ((static_cast<unsigned char>(text[index + part]) & 0xc0) != 0x80)
            {
                fail("invalid_utf8", "文本包含无效的 UTF-8 字节序列");
            }
        }
        const auto second = static_cast<unsigned char>(text[index + 1]);
        if ((first == 0xe0 && second < 0xa0) ||
            (first == 0xed && second >= 0xa0) ||
            (first == 0xf0 && second < 0x90) ||
            (first == 0xf4 && second >= 0x90))
        {
            fail("invalid_utf8", "文本包含无效的 UTF-8 字节序列");
        }
        index += count;
    }
}

void validate_token(std::string_view text, std::string_view description)
{
    constexpr std::string_view punctuation = "!#$%&'*+-.^_`|~";
    if (text.empty() || !std::all_of(text.begin(), text.end(), [&](unsigned char value)
        {
            return (value >= '0' && value <= '9') ||
                (value >= 'A' && value <= 'Z') ||
                (value >= 'a' && value <= 'z') ||
                punctuation.find(static_cast<char>(value)) != std::string_view::npos;
        }))
    {
        fail("invalid_header", std::string(description) + "不符合 HTTP token 规则");
    }
}

void validate_header(std::string_view name, std::string_view value)
{
    validate_token(name, "HTTP 头名称");
    if (value.find_first_of("\r\n\0", 0, 3) != std::string_view::npos)
    {
        fail("invalid_header", "HTTP 头值含有 CR、LF 或 NUL");
    }
    validate_utf8(value);
}

std::string lower_ascii(std::string_view text)
{
    std::string result(text);
    for (char& value : result)
    {
        if (value >= 'A' && value <= 'Z')
        {
            value = static_cast<char>(value + ('a' - 'A'));
        }
    }
    return result;
}

parsed_head parse_head(std::string_view text)
{
    if (text.size() > max_head_bytes || !text.ends_with("\r\n\r\n"))
    {
        fail("protocol_error", "HTTP 头未完整结束或超过 64 KiB");
    }
    parsed_head result;
    const auto first_end = text.find("\r\n");
    result.first_line = std::string(text.substr(0, first_end));
    validate_utf8(result.first_line);
    std::size_t position = first_end + 2;
    while (position + 2 < text.size())
    {
        const auto end = text.find("\r\n", position);
        const auto line = text.substr(position, end - position);
        if (line.empty())
        {
            break;
        }
        const auto colon = line.find(':');
        if (colon == std::string_view::npos)
        {
            fail("protocol_error", "HTTP 头缺少冒号");
        }
        auto name = lower_ascii(line.substr(0, colon));
        auto value = trim_ows(line.substr(colon + 1));
        validate_header(name, value);
        if (name == "set-cookie")
        {
            result.cookies.push_back(std::move(value));
        }
        else if (auto [item, inserted] = result.headers.emplace(name, value); !inserted)
        {
            if (name == "content-length" || name == "host" ||
                name == "transfer-encoding" || name == "connection" ||
                name == "www-authenticate" || name == "proxy-authenticate")
            {
                fail("protocol_error", "HTTP 头存在不能安全合并的重复字段：" + name);
            }
            item->second += name == "cookie" ? "; " : ", ";
            item->second += value;
        }
        position = end + 2;
    }
    return result;
}

std::size_t content_length(const header_map& headers, std::size_t max_length)
{
    const auto found = headers.find("content-length");
    if (found == headers.end())
    {
        return 0;
    }
    std::size_t size = 0;
    const auto& text = found->second;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), size);
    if (error != std::errc{} || end != text.data() + text.size())
    {
        fail("protocol_error", "Content-Length 不是有效的非负整数");
    }
    if (size > max_length)
    {
        fail("size_limit", "HTTP 正文超过允许的长度");
    }
    return size;
}

socket_handle::socket_handle(socket_handle&& other) noexcept
    : value_(std::exchange(other.value_, INVALID_SOCKET))
{
}

socket_handle& socket_handle::operator=(socket_handle&& other) noexcept
{
    reset(std::exchange(other.value_, INVALID_SOCKET));
    return *this;
}

socket_handle::~socket_handle()
{
    reset();
}

void socket_handle::reset(SOCKET value) noexcept
{
    if (valid())
    {
        closesocket(value_);
    }
    value_ = value;
}

http_handle::http_handle(http_handle&& other) noexcept
    : value_(std::exchange(other.value_, nullptr))
{
}

http_handle& http_handle::operator=(http_handle&& other) noexcept
{
    reset(std::exchange(other.value_, nullptr));
    return *this;
}

http_handle::~http_handle()
{
    reset();
}

void http_handle::reset(HINTERNET value) noexcept
{
    if (value_)
    {
        WinHttpCloseHandle(value_);
    }
    value_ = value;
}

[[noreturn]] void socket_failure(std::string_view action)
{
    const int code = WSAGetLastError();
    fail(code == WSAETIMEDOUT || code == WSAEWOULDBLOCK ? "timeout" : "operation_failed",
         std::string(action) + "失败，Winsock 错误码 " + std::to_string(code));
}

[[noreturn]] void http_failure(std::string_view action)
{
    const auto code = GetLastError();
    fail(code == ERROR_WINHTTP_TIMEOUT ? "timeout" : "operation_failed",
         std::string(action) + "失败，WinHTTP 错误码 " + std::to_string(code));
}

void tcp_stream::set_receive_timeout(std::int64_t timeout_ms)
{
    if (timeout_ms < 0 || timeout_ms > std::numeric_limits<DWORD>::max())
    {
        fail("invalid_argument", "超时参数超出允许范围");
    }
    receive_timeout_ms_ = timeout_ms;
    if (secure_)
    {
        return;
    }
    const DWORD value = static_cast<DWORD>(timeout_ms);
    if (setsockopt(socket_.get(), SOL_SOCKET, SO_RCVTIMEO,
                   reinterpret_cast<const char*>(&value), sizeof(value)) != 0)
    {
        socket_failure("设置读取超时");
    }
}

void tcp_stream::set_receive_deadline(std::int64_t timeout_ms)
{
    if (timeout_ms < 0 || timeout_ms > std::numeric_limits<DWORD>::max())
    {
        fail("invalid_argument", "超时参数超出允许范围");
    }
    if (timeout_ms == 0)
    {
        receive_deadline_.reset();
        return;
    }
    receive_deadline_ = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeout_ms);
}

void tcp_stream::clear_receive_deadline()
{
    receive_deadline_.reset();
    set_receive_timeout(receive_timeout_ms_);
}

bool tcp_stream::valid() const noexcept
{
    return secure_ ? !secure_->closed() : socket_.valid();
}

void tcp_stream::close() noexcept
{
    if (secure_)
    {
        try
        {
            secure_->close(1000);
        }
        catch (...)
        {
        }
        secure_.reset();
    }
    socket_.reset();
}

std::string tcp_stream::read_some(std::size_t limit)
{
    std::int64_t timeout_ms = receive_timeout_ms_;
    if (receive_deadline_)
    {
        const auto remaining = *receive_deadline_ - std::chrono::steady_clock::now();
        if (remaining <= std::chrono::steady_clock::duration::zero())
        {
            fail("timeout", "读取 HTTP 请求的总时限已到");
        }
        timeout_ms = std::max<std::int64_t>(1,
            std::chrono::duration_cast<std::chrono::milliseconds>(remaining).count());
        if (receive_timeout_ms_ != 0)
        {
            timeout_ms = std::min(timeout_ms, receive_timeout_ms_);
        }
        if (!secure_)
        {
            const DWORD value = static_cast<DWORD>(timeout_ms);
            if (setsockopt(socket_.get(), SOL_SOCKET, SO_RCVTIMEO,
                           reinterpret_cast<const char*>(&value), sizeof(value)) != 0)
            {
                socket_failure("设置读取超时");
            }
        }
    }
    if (secure_)
    {
        while (true)
        {
            try
            {
                auto value = secure_->read(static_cast<std::int64_t>(limit),
                    timeout_ms == 0 ? 30000 : timeout_ms);
                if (value.eof)
                {
                    return {};
                }
                if (value.data.empty())
                {
                    continue;
                }
                return {reinterpret_cast<const char*>(value.data.data()),
                    value.data.size()};
            }
            catch (const runtime_failure& error)
            {
                if (receive_timeout_ms_ != 0 || error.error().code != "timeout")
                {
                    throw;
                }
            }
        }
    }
    std::string result(limit, '\0');
    const int count = recv(socket_.get(), result.data(),
        static_cast<int>(limit), 0);
    if (count < 0)
    {
        socket_failure("读取数据");
    }
    result.resize(static_cast<std::size_t>(count));
    return result;
}

std::size_t tcp_stream::send_some(std::string_view data)
{
    if (secure_)
    {
        return static_cast<std::size_t>(secure_->write(data, 30000));
    }
    const int sent = send(socket_.get(), data.data(),
        static_cast<int>(data.size()), 0);
    if (sent < 0)
    {
        socket_failure("发送数据");
    }
    return static_cast<std::size_t>(sent);
}

std::string tcp_stream::read_head()
{
    while (true)
    {
        const auto end = pending_.find("\r\n\r\n");
        if (end != std::string::npos)
        {
            if (end + 4 > max_head_bytes)
            {
                fail("size_limit", "HTTP 头超过 64 KiB");
            }
            auto result = pending_.substr(0, end + 4);
            pending_.erase(0, end + 4);
            return result;
        }
        if (pending_.size() >= max_head_bytes)
        {
            fail("size_limit", "HTTP 头超过 64 KiB");
        }
        auto block = read_some(4096);
        if (block.empty())
        {
            fail("connection_closed", "读取 HTTP 头时连接已关闭");
        }
        pending_ += block;
    }
}

std::string tcp_stream::read_exact(std::size_t length)
{
    std::string result;
    result.reserve(length);
    const auto available = std::min(length, pending_.size());
    result.append(pending_.data(), available);
    pending_.erase(0, available);
    while (result.size() < length)
    {
        const auto amount = std::min<std::size_t>(length - result.size(), 8192);
        auto block = read_some(amount);
        if (block.empty())
        {
            fail("connection_closed", "读取数据时连接已关闭");
        }
        result += block;
    }
    return result;
}

void tcp_stream::send_all(std::string_view data)
{
    while (!data.empty())
    {
        const auto amount = std::min(data.size(),
            static_cast<std::size_t>(std::numeric_limits<int>::max()));
        const auto sent = send_some(data.substr(0, amount));
        if (sent == 0)
        {
            fail("connection_closed", "发送数据时连接已关闭");
        }
        data.remove_prefix(sent);
    }
}

} // namespace tx_generated::network
