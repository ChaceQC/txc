#include "stdlib/env.hpp"

#include "stdlib/encoding.hpp"

#include <cwchar>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace tx_generated
{

tx_dict tx_fn_env_snapshot()
{
    std::unique_ptr<wchar_t, decltype(&FreeEnvironmentStringsW)> block(
        GetEnvironmentStringsW(), &FreeEnvironmentStringsW);
    if (!block)
    {
        throw std::runtime_error("读取进程环境快照失败（系统错误 " +
            std::to_string(GetLastError()) + "）");
    }
    tx_dict result;
    for (const wchar_t* entry = block.get(); *entry != L'\0';
         entry += std::wcslen(entry) + 1)
    {
        const std::wstring_view item(entry);
        // Windows 的 =C: 等隐藏条目记录盘符工作目录，不属于普通环境变量。
        if (item.starts_with(L"="))
        {
            continue;
        }
        const auto separator = item.find(L'=');
        if (separator == std::wstring_view::npos)
        {
            continue;
        }
        result.emplace_back(detail::wide_to_utf8(item.substr(0, separator)),
            detail::wide_to_utf8(item.substr(separator + 1)));
    }
    return result;
}

} // namespace tx_generated
