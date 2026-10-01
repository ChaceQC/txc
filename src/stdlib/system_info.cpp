#include "stdlib/system.hpp"

#include "stdlib/error.hpp"

#include <stdexcept>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "common/platform.hpp"
#include <thread>
#ifdef _WIN32
#include <windows.h>
#endif

namespace tx_generated
{

std::string tx_fn_system_operating_system()
{
#ifdef _WIN32
    return "windows";
#else
    return "linux";
#endif
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
#ifdef _WIN32
    const auto count = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
#else
    const auto count = std::thread::hardware_concurrency();
#endif
    if (count == 0)
    {
        throw std::runtime_error("无法查询活动逻辑处理器数量");
    }
    return count;
}

std::int64_t tx_fn_system_process_id()
{
    return tx::process_id();
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
