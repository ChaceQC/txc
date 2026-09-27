#include "stdlib/system.hpp"

#include "stdlib/error.hpp"

#include <stdexcept>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace tx_generated
{

std::string tx_fn_system_operating_system()
{
    return "windows";
}

std::string tx_fn_system_architecture()
{
#if defined(__x86_64__) || defined(_M_X64)
    return "x86_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
    return "aarch64";
#else
    throw std::runtime_error("当前构建未登记目标架构");
#endif
}

std::int64_t tx_fn_system_cpu_count()
{
    const auto count = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
    if (count == 0)
    {
        throw std::runtime_error("无法查询活动逻辑处理器数量");
    }
    return count;
}

std::int64_t tx_fn_system_process_id()
{
    return GetCurrentProcessId();
}

bool tx_fn_system_has_capability(const std::string& name)
{
    if (name == "atomic_replace" || name == "file_sync" ||
        name == "file_lock" || name == "file_watch" || name == "symlink")
    {
        return true;
    }
    if (name == "process_spawn")
    {
        return true;
    }
    throw runtime_failure({tx::error_kind::runtime, "invalid_argument",
        "未知系统能力名称"});
}

} // namespace tx_generated
