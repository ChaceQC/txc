#include "stdlib/network_common.hpp"
#include "stdlib/encoding.hpp"

#include <curl/curl.h>
#include <openssl/evp.h>
#include <charconv>
#include <memory>

namespace tx_generated::network
{
namespace
{
std::string url_part(CURLU* url, CURLUPart part, unsigned flags = 0)
{
    char* value = nullptr;
    const auto status = curl_url_get(url, part, &value, flags);
    std::unique_ptr<char, decltype(&curl_free)> guard(value, curl_free);
    return status == CURLUE_OK ? std::string(value) : std::string{};
}
}

parsed_url parse_url(std::string_view text, bool websocket)
{
    validate_utf8(text);
    if (text.find_first_of("\0\r\n\t #", 0, 6) != std::string_view::npos)
    {
        fail("invalid_url", "URL 包含控制字符、空格或片段标识符");
    }
    const bool secure = text.starts_with(websocket ? "wss://" : "https://");
    if (!secure && !text.starts_with(websocket ? "ws://" : "http://"))
    {
        fail("invalid_url", "URL 协议不受支持");
    }
    const std::string converted = websocket
        ? std::string(secure ? "https://" : "http://") +
            std::string(text.substr(secure ? 6 : 5)) : std::string(text);
    std::unique_ptr<CURLU, decltype(&curl_url_cleanup)> url(curl_url(), curl_url_cleanup);
    if (!url || curl_url_set(url.get(), CURLUPART_URL, converted.c_str(), CURLU_PATH_AS_IS) != CURLUE_OK)
    {
        fail("invalid_url", "URL 缺少有效主机或端口");
    }
    if (!url_part(url.get(), CURLUPART_USER).empty() ||
        !url_part(url.get(), CURLUPART_PASSWORD).empty() ||
        converted.substr(converted.find("://") + 3,
            converted.find_first_of("/?", converted.find("://") + 3) -
            (converted.find("://") + 3)).find('@') != std::string::npos)
    {
        fail("invalid_url", "URL 不能包含用户信息");
    }
    auto host = url_part(url.get(), CURLUPART_HOST, CURLU_PUNYCODE);
    if (host.empty())
    {
        fail("invalid_url", "URL 主机无效");
    }
    if (host.starts_with('[') && host.ends_with(']'))
    {
        host = host.substr(1, host.size() - 2);
    }
    const auto port_text = url_part(url.get(), CURLUPART_PORT, CURLU_DEFAULT_PORT);
    unsigned port = 0;
    const auto [end, error] = std::from_chars(port_text.data(), port_text.data() + port_text.size(), port);
    if (error != std::errc{} || end != port_text.data() + port_text.size() || port == 0 || port > 65535)
    {
        fail("invalid_url", "URL 端口无效");
    }
    auto target = url_part(url.get(), CURLUPART_PATH);
    if (target.empty())
    {
        target = "/";
    }
    const auto query = url_part(url.get(), CURLUPART_QUERY);
    if (!query.empty())
    {
        target += "?" + query;
    }
    return {detail::utf8_to_wide(host), detail::utf8_to_wide(target),
        static_cast<std::uint16_t>(port), secure};
}

std::string base64(std::string_view bytes)
{
    const auto length = 4 * ((bytes.size() + 2) / 3);
    std::string result(length + 1, '\0');
    if (!bytes.empty())
    {
        EVP_EncodeBlock(reinterpret_cast<unsigned char*>(result.data()),
            reinterpret_cast<const unsigned char*>(bytes.data()),
            static_cast<int>(bytes.size()));
    }
    result.resize(length);
    return result;
}

std::string sha1(std::string_view bytes)
{
    std::string result(20, '\0');
    unsigned length = 0;
    if (EVP_Digest(bytes.data(), bytes.size(), reinterpret_cast<unsigned char*>(result.data()), &length,
        EVP_sha1(), nullptr) != 1 || length != result.size())
    {
        fail("operation_failed", "计算 WebSocket 握手哈希失败");
    }
    return result;
}
}
