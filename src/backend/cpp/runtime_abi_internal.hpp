#pragma once

#include <cstdio>
#include <exception>
#include <utility>

namespace tx_generated::detail
{

extern thread_local char last_error[256];

template<class operation>
int invoke_checked(operation&& run) noexcept
{
    try
    {
        std::forward<operation>(run)();
        last_error[0] = '\0';
        return 0;
    }
    catch (const std::exception& error)
    {
        std::snprintf(last_error, 256, "%s", error.what());
    }
    catch (...)
    {
        std::snprintf(last_error, 256, "%s", "未知运行时错误");
    }
    return 1;
}

} // namespace tx_generated::detail
