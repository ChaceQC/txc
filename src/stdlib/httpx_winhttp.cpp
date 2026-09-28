#include "stdlib/httpx_winhttp.hpp"

#include "stdlib/encoding.hpp"

namespace tx_generated::httpx_winhttp
{
namespace
{

network::parsed_head read_response_head(HINTERNET request)
{
    DWORD bytes = 0;
    if (WinHttpQueryHeaders(request, WINHTTP_QUERY_RAW_HEADERS_CRLF,
                            WINHTTP_HEADER_NAME_BY_INDEX, nullptr, &bytes,
                            WINHTTP_NO_HEADER_INDEX) ||
        GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        network::http_failure("读取 HTTP 响应头长度");
    }
    if (bytes > network::max_head_bytes * sizeof(wchar_t))
    {
        network::fail("size_limit", "HTTP 响应头超过 64 KiB");
    }
    std::wstring wide(bytes / sizeof(wchar_t), L'\0');
    if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_RAW_HEADERS_CRLF,
                             WINHTTP_HEADER_NAME_BY_INDEX, wide.data(), &bytes,
                             WINHTTP_NO_HEADER_INDEX))
    {
        network::http_failure("读取 HTTP 响应头");
    }
    while (!wide.empty() && wide.back() == L'\0')
    {
        wide.pop_back();
    }
    const auto text = detail::wide_to_utf8(wide);
    if (text.size() > network::max_head_bytes)
    {
        network::fail("size_limit", "HTTP 响应头超过 64 KiB");
    }
    return network::parse_head(text);
}

} // namespace

std::wstring request_headers(const network::header_map& headers)
{
    std::wstring result;
    for (const auto& [name, value] : headers)
    {
        network::validate_header(name, value);
        if (name == "host" || name == "content-length" ||
            name == "transfer-encoding" || name == "connection" ||
            name == "proxy-connection" || name == "keep-alive" ||
            name == "upgrade")
        {
            network::fail("invalid_header", "调用方不能覆盖 HTTP 连接管理字段");
        }
        result += detail::utf8_to_wide(name + ": " + value + "\r\n");
        if (result.size() * sizeof(wchar_t) > network::max_head_bytes)
        {
            network::fail("size_limit", "HTTP 请求头超过 64 KiB");
        }
    }
    return result;
}

http_response_data response_metadata(HINTERNET request)
{
    http_response_data result;
    DWORD status = 0;
    DWORD size = sizeof(status);
    if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE |
                             WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &status, &size,
                             WINHTTP_NO_HEADER_INDEX))
    {
        network::http_failure("读取 HTTP 状态码");
    }
    result.status = status;
    DWORD used = 0;
    DWORD used_size = sizeof(used);
    result.protocol = WinHttpQueryOption(request,
        WINHTTP_OPTION_HTTP_PROTOCOL_USED, &used, &used_size) &&
        (used & WINHTTP_PROTOCOL_FLAG_HTTP2) != 0 ? "h2" : "http/1.1";
    auto head = read_response_head(request);
    result.headers = std::move(head.headers);
    result.cookies = std::move(head.cookies);
    return result;
}

void require_http2_protocol(HINTERNET request)
{
    DWORD used = 0;
    DWORD size = sizeof(used);
    if (!WinHttpQueryOption(request, WINHTTP_OPTION_HTTP_PROTOCOL_USED,
                            &used, &size) ||
        (used & WINHTTP_PROTOCOL_FLAG_HTTP2) == 0)
    {
        network::fail("protocol_error", "HTTPS 服务端未协商 HTTP/2");
    }
}

} // namespace tx_generated::httpx_winhttp
