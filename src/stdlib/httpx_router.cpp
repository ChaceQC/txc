#include "stdlib/httpx_router.hpp"

#include <vector>

namespace tx_generated::httpx_router
{
namespace
{

std::vector<std::string_view> segments(std::string_view path)
{
    std::vector<std::string_view> result;
    std::size_t start = 1;
    while (start <= path.size())
    {
        if (result.size() == 64)
        {
            network::fail("size_limit", "HTTP 路由路径超过 64 段");
        }
        const auto end = path.find('/', start);
        result.push_back(path.substr(start, end == std::string_view::npos
            ? end : end - start));
        if (end == std::string_view::npos)
        {
            break;
        }
        start = end + 1;
    }
    return result;
}

bool parameter(std::string_view segment)
{
    return segment.size() >= 3 && segment.front() == '{' &&
           segment.back() == '}';
}

} // namespace

match_result match(std::string_view route_method, std::string_view pattern,
                   std::string_view request_method, std::string_view target)
{
    if (route_method != "*")
    {
        network::validate_token(route_method, "HTTP 路由方法");
    }
    network::validate_token(request_method, "HTTP 请求方法");
    network::validate_utf8(pattern);
    network::validate_utf8(target);
    if (pattern.empty() || pattern.front() != '/' ||
        target.empty() || target.front() != '/' ||
        pattern.size() > 8192 || target.size() > 8192)
    {
        network::fail("invalid_argument", "HTTP 路由模式或请求目标无效");
    }
    if (pattern.find('?') != std::string_view::npos ||
        pattern.find('#') != std::string_view::npos)
    {
        network::fail("invalid_argument", "HTTP 路由模式不能包含查询或片段");
    }
    match_result result;
    if (route_method != "*" && route_method != request_method)
    {
        return result;
    }
    const auto path = target.substr(0, target.find('?'));
    const auto patterns = segments(pattern);
    const auto parts = segments(path);
    if (patterns.size() != parts.size())
    {
        return result;
    }
    for (std::size_t index = 0; index < patterns.size(); ++index)
    {
        const auto expected = patterns[index];
        if (parameter(expected))
        {
            const auto key = expected.substr(1, expected.size() - 2);
            network::validate_token(key, "HTTP 路由参数名");
            if (parts[index].empty() || result.params.size() == 16 ||
                !result.params.emplace(key, parts[index]).second)
            {
                network::fail("invalid_argument", "HTTP 路由参数为空、重复或超过 16 个");
            }
        }
        else if (expected != parts[index])
        {
            return {};
        }
    }
    result.matched = true;
    return result;
}

} // namespace tx_generated::httpx_router
