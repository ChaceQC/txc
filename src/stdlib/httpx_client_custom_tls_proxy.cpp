#include "stdlib/httpx_client_custom_tls_transport.hpp"

#include "stdlib/encoding.hpp"
#include "stdlib/error.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace tx_generated::httpx_custom_tls
{
namespace
{
std::wstring lower_wide(std::wstring_view value)
{
    std::wstring result(value);
    for (auto& character : result)
    {
        if (character >= L'A' && character <= L'Z')
        {
            character += L'a' - L'A';
        }
    }
    return result;
}

wchar_t lower_wide_character(wchar_t value) noexcept
{
    if (value >= L'A' && value <= L'Z')
    {
        return value + (L'a' - L'A');
    }
    return value;
}

bool wildcard_match(std::wstring_view pattern, std::wstring_view value)
{
    std::size_t pattern_index = 0;
    std::size_t value_index = 0;
    std::size_t star_index = std::wstring_view::npos;
    std::size_t retry_index = 0;
    while (value_index < value.size())
    {
        if (pattern_index < pattern.size() &&
            (pattern[pattern_index] == L'?' ||
             lower_wide_character(pattern[pattern_index]) ==
                lower_wide_character(value[value_index])))
        {
            ++pattern_index;
            ++value_index;
        }
        else if (pattern_index < pattern.size() && pattern[pattern_index] == L'*')
        {
            star_index = pattern_index++;
            retry_index = value_index;
        }
        else if (star_index != std::wstring_view::npos)
        {
            pattern_index = star_index + 1;
            value_index = ++retry_index;
        }
        else
        {
            return false;
        }
    }
    while (pattern_index < pattern.size() && pattern[pattern_index] == L'*')
    {
        ++pattern_index;
    }
    return pattern_index == pattern.size();
}

bool bypass_matches(std::wstring_view host, std::wstring_view bypasses)
{
    const auto normalized_host = lower_wide(host);
    host = normalized_host;
    std::size_t start = 0;
    while (start < bypasses.size())
    {
        const auto end = bypasses.find_first_of(L"; ,", start);
        const auto item = bypasses.substr(start,
            end == std::wstring_view::npos ? bypasses.size() - start
                                           : end - start);
        if (!item.empty())
        {
            const auto pattern = lower_wide(item);
            if (pattern == L"<local>" ? host.find(L'.') == std::wstring_view::npos
                                      : wildcard_match(pattern, host))
            {
                return true;
            }
        }
        if (end == std::wstring_view::npos)
        {
            break;
        }
        start = end + 1;
    }
    return false;
}

std::optional<std::wstring> select_proxy(std::wstring_view proxies,
                                         bool secure)
{
    std::optional<std::wstring> fallback;
    std::optional<std::wstring> scheme_proxy;
    std::size_t start = 0;
    while (start < proxies.size())
    {
        const auto end = proxies.find(';', start);
        auto item = proxies.substr(start,
            end == std::wstring_view::npos ? proxies.size() - start
                                           : end - start);
        while (!item.empty() && item.front() == L' ')
        {
            item.remove_prefix(1);
        }
        while (!item.empty() && item.back() == L' ')
        {
            item.remove_suffix(1);
        }
        if (!item.empty())
        {
            const auto equals = item.find('=');
            if (equals == std::wstring_view::npos)
            {
                fallback = std::wstring(item);
            }
            else
            {
                const auto protocol = lower_wide(item.substr(0, equals));
                if ((secure && protocol == L"https") ||
                    (!secure && protocol == L"http"))
                {
                    scheme_proxy = std::wstring(item.substr(equals + 1));
                }
                else if (secure && protocol == L"http")
                {
                    fallback = std::wstring(item.substr(equals + 1));
                }
            }
        }
        if (end == std::wstring_view::npos)
        {
            break;
        }
        start = end + 1;
    }
    return scheme_proxy ? scheme_proxy : fallback;
}

struct ie_proxy_config
{
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG value{};

    ~ie_proxy_config()
    {
        if (value.lpszProxy)
        {
            GlobalFree(value.lpszProxy);
        }
        if (value.lpszProxyBypass)
        {
            GlobalFree(value.lpszProxyBypass);
        }
        if (value.lpszAutoConfigUrl)
        {
            GlobalFree(value.lpszAutoConfigUrl);
        }
    }
};

struct proxy_info
{
    WINHTTP_PROXY_INFO value{};

    ~proxy_info()
    {
        if (value.lpszProxy)
        {
            GlobalFree(value.lpszProxy);
        }
        if (value.lpszProxyBypass)
        {
            GlobalFree(value.lpszProxyBypass);
        }
    }
};

std::string proxy_url_from_info(const WINHTTP_PROXY_INFO& info,
    std::wstring_view host, bool secure)
{
    if (info.dwAccessType == WINHTTP_ACCESS_TYPE_NO_PROXY ||
        bypass_matches(host, info.lpszProxyBypass ? info.lpszProxyBypass : L""))
    {
        return "direct";
    }
    if (info.dwAccessType != WINHTTP_ACCESS_TYPE_NAMED_PROXY || !info.lpszProxy)
    {
        return "direct";
    }
    const auto selected = select_proxy(info.lpszProxy, secure);
    if (!selected || selected->empty())
    {
        return "direct";
    }
    if (selected->starts_with(L"http://") || selected->starts_with(L"https://"))
    {
        return detail::wide_to_utf8(*selected);
    }
    return "http://" + detail::wide_to_utf8(*selected);
}

std::string system_proxy_url(HINTERNET session,
    std::wstring_view target_host, std::string_view target_url, bool secure)
{
    ie_proxy_config current_user;
    if (WinHttpGetIEProxyConfigForCurrentUser(&current_user.value))
    {
        if (current_user.value.lpszAutoConfigUrl ||
            current_user.value.fAutoDetect)
        {
            WINHTTP_AUTOPROXY_OPTIONS options{};
            if (current_user.value.lpszAutoConfigUrl)
            {
                options.dwFlags |= WINHTTP_AUTOPROXY_CONFIG_URL;
                options.lpszAutoConfigUrl = current_user.value.lpszAutoConfigUrl;
            }
            if (current_user.value.fAutoDetect)
            {
                options.dwFlags |= WINHTTP_AUTOPROXY_AUTO_DETECT;
                options.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP |
                                            WINHTTP_AUTO_DETECT_TYPE_DNS_A;
            }
            options.fAutoLogonIfChallenged = FALSE;
            proxy_info resolved;
            const auto wide_url = detail::utf8_to_wide(target_url);
            if (!WinHttpGetProxyForUrl(session, wide_url.c_str(), &options,
                                       &resolved.value))
            {
                network::http_failure("解析系统 HTTP 代理");
            }
            return proxy_url_from_info(resolved.value, target_host, secure);
        }
        if (current_user.value.lpszProxy)
        {
            WINHTTP_PROXY_INFO configured{};
            configured.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
            configured.lpszProxy = current_user.value.lpszProxy;
            configured.lpszProxyBypass = current_user.value.lpszProxyBypass;
            return proxy_url_from_info(configured, target_host, secure);
        }
    }
    proxy_info configured;
    if (WinHttpGetDefaultProxyConfiguration(&configured.value))
    {
        const auto selected = proxy_url_from_info(configured.value,
                                                   target_host, secure);
        if (selected != "direct")
        {
            return selected;
        }
    }
    return "direct";
}
} // namespace

std::string resolve_system_proxy(HINTERNET session,
    std::wstring_view target_host, std::string_view target_url, bool secure)
{
    return system_proxy_url(session, target_host, target_url, secure);
}

} // namespace tx_generated::httpx_custom_tls
