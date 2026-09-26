#include "stdlib/network_common.hpp"

#include "stdlib/encoding.hpp"

#include <limits>
#include <wincrypt.h>

namespace tx_generated::network
{
namespace
{

struct crypto_provider
{
    HCRYPTPROV value = 0;
    crypto_provider()
    {
        if (!CryptAcquireContextW(&value, nullptr, nullptr, PROV_RSA_FULL,
                                  CRYPT_VERIFYCONTEXT))
        {
            fail("operation_failed", "无法初始化系统加密提供程序");
        }
    }
    ~crypto_provider()
    {
        CryptReleaseContext(value, 0);
    }
};

} // namespace

parsed_url parse_url(std::string_view text, bool websocket)
{
    validate_utf8(text);
    if (text.find('\0') != std::string_view::npos ||
        text.find('#') != std::string_view::npos)
    {
        fail("invalid_url", "URL 不能包含 NUL 或片段标识符");
    }
    const bool secure = websocket ? text.starts_with("wss://") : text.starts_with("https://");
    const bool plain = websocket ? text.starts_with("ws://") : text.starts_with("http://");
    if (!secure && !plain)
    {
        fail("invalid_url", "URL 协议不受支持");
    }
    const auto converted = websocket
        ? std::string(secure ? "https://" : "http://") +
            std::string(text.substr(secure ? 6 : 5)) : std::string(text);
    const auto wide = detail::utf8_to_wide(converted);
    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);
    parts.dwHostNameLength = static_cast<DWORD>(-1);
    parts.dwUrlPathLength = static_cast<DWORD>(-1);
    parts.dwExtraInfoLength = static_cast<DWORD>(-1);
    parts.dwUserNameLength = static_cast<DWORD>(-1);
    parts.dwPasswordLength = static_cast<DWORD>(-1);
    if (!WinHttpCrackUrl(wide.c_str(), static_cast<DWORD>(wide.size()), 0, &parts) ||
        parts.dwHostNameLength == 0 || parts.dwUserNameLength != 0 ||
        parts.dwPasswordLength != 0)
    {
        fail("invalid_url", "URL 缺少有效主机或包含不支持的用户信息");
    }
    parsed_url result;
    result.host.assign(parts.lpszHostName, parts.dwHostNameLength);
    result.target = parts.dwUrlPathLength != 0
        ? std::wstring(parts.lpszUrlPath, parts.dwUrlPathLength) : L"/";
    if (parts.dwExtraInfoLength != 0)
    {
        result.target.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);
    }
    result.port = parts.nPort;
    result.secure = secure;
    return result;
}

std::string base64(std::string_view bytes)
{
    constexpr char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    result.reserve(((bytes.size() + 2) / 3) * 4);
    for (std::size_t index = 0; index < bytes.size(); index += 3)
    {
        const auto first = static_cast<unsigned char>(bytes[index]);
        const auto second = index + 1 < bytes.size()
            ? static_cast<unsigned char>(bytes[index + 1]) : 0;
        const auto third = index + 2 < bytes.size()
            ? static_cast<unsigned char>(bytes[index + 2]) : 0;
        result.push_back(alphabet[first >> 2]);
        result.push_back(alphabet[((first & 3) << 4) | (second >> 4)]);
        result.push_back(index + 1 < bytes.size()
            ? alphabet[((second & 15) << 2) | (third >> 6)] : '=');
        result.push_back(index + 2 < bytes.size() ? alphabet[third & 63] : '=');
    }
    return result;
}

std::string sha1(std::string_view bytes)
{
    crypto_provider provider;
    HCRYPTHASH hash = 0;
    if (!CryptCreateHash(provider.value, CALG_SHA1, 0, 0, &hash))
    {
        fail("operation_failed", "无法创建 SHA-1 哈希");
    }
    struct hash_guard
    {
        HCRYPTHASH value;
        ~hash_guard() { CryptDestroyHash(value); }
    } guard{hash};
    if (bytes.size() > std::numeric_limits<DWORD>::max() ||
        !CryptHashData(hash, reinterpret_cast<const BYTE*>(bytes.data()),
                       static_cast<DWORD>(bytes.size()), 0))
    {
        fail("operation_failed", "计算 WebSocket 握手哈希失败");
    }
    DWORD size = 20;
    std::string result(size, '\0');
    if (!CryptGetHashParam(hash, HP_HASHVAL,
                           reinterpret_cast<BYTE*>(result.data()), &size, 0))
    {
        fail("operation_failed", "读取 WebSocket 握手哈希失败");
    }
    return result;
}

} // namespace tx_generated::network
