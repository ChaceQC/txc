#include "stdlib/process_internal.hpp"
#include "stdlib/error.hpp"

#include <algorithm>

namespace tx_generated::process_detail
{

[[noreturn]] void fail(const char* code, const char* message, DWORD error)
{
    std::string text(message);
    if (error != 0)
    {
        text += "（系统错误 " + std::to_string(error) + "）";
    }
    throw runtime_failure({tx::error_kind::process, code, std::move(text)});
}

void require_child(const process_child& child)
{
    if (!child || child->closed)
    {
        fail("invalid_state", "子进程句柄已关闭");
    }
}

void validate_timeout(std::int64_t timeout_ms)
{
    if (timeout_ms < -1 || timeout_ms > 0xfffffffeLL)
    {
        fail("invalid_argument", "进程超时必须为 -1 或 0～4294967294 毫秒");
    }
}

steady_clock::time_point deadline(std::int64_t timeout_ms)
{
    validate_timeout(timeout_ms);
    return timeout_ms < 0 ? steady_clock::time_point::max()
        : steady_clock::now() + std::chrono::milliseconds(timeout_ms);
}

std::string cancellation_reason(const std::shared_ptr<cancellation_state>& state)
{
    if (!state)
    {
        return {};
    }
    std::lock_guard lock(state->mutex);
    if (state->cancelled)
    {
        return "cancelled";
    }
    if (state->deadline && steady_clock::now() >= *state->deadline)
    {
        return "deadline_exceeded";
    }
    return {};
}

bool poll_exit(process_child_state& child)
{
    if (child.completed)
    {
        return true;
    }
    const auto state = WaitForSingleObject(child.handle.get(), 0);
    if (state == WAIT_TIMEOUT)
    {
        return false;
    }
    DWORD code = 0;
    if (state != WAIT_OBJECT_0 || !GetExitCodeProcess(child.handle.get(), &code))
    {
        fail("wait_failed", "查询子进程退出状态失败", GetLastError());
    }
    // STILL_ACTIVE 也可作为程序真实退出码；由句柄信号判定是否退出。
    const bool terminated = (child.killed && code == 0xe0000001UL) ||
        code >= 0xc0000000UL || code == 0x80000003UL;
    child.status = {terminated ? "terminated" : "exited", code};
    child.completed = true;
    child.handle.reset();
    return true;
}

} // namespace tx_generated::process_detail

namespace tx_generated
{

std::int64_t process_id(const process_child& child)
{
    process_detail::require_child(child);
    return child->id;
}

process_status process_wait(const process_child& child, std::int64_t timeout_ms,
    const std::shared_ptr<cancellation_state>& cancellation)
{
    process_detail::require_child(child);
    const auto limit = process_detail::deadline(timeout_ms);
    while (!process_detail::poll_exit(*child))
    {
        const auto reason = process_detail::cancellation_reason(cancellation);
        if (!reason.empty())
        {
            throw runtime_failure({tx::error_kind::cancelled, reason,
                "进程等待已取消；子进程仍由调用方管理"});
        }
        const auto now = process_detail::steady_clock::now();
        if (now >= limit)
        {
            return {timeout_ms == 0 ? "running" : "timeout", 0};
        }
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            limit - now).count();
        const auto slice = static_cast<DWORD>(std::clamp<std::int64_t>(remaining, 1, 10));
        if (WaitForSingleObject(child->handle.get(), slice) == WAIT_FAILED)
        {
            process_detail::fail("wait_failed", "等待子进程失败", GetLastError());
        }
    }
    return child->status;
}

void process_terminate(const process_child& child)
{
    process_detail::require_child(child);
    if (process_detail::poll_exit(*child))
    {
        return;
    }
    if (!child->group)
    {
        process_detail::fail("unsupported_operation", "协作终止需要独立控制台进程组");
    }
    if (!GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, child->id))
    {
        process_detail::fail("unsupported_operation", "当前控制台不支持协作终止", GetLastError());
    }
}

void process_kill(const process_child& child)
{
    process_detail::require_child(child);
    if (process_detail::poll_exit(*child))
    {
        return;
    }
    if (!TerminateProcess(child->handle.get(), 0xe0000001UL))
    {
        const auto error = GetLastError();
        if (process_detail::poll_exit(*child))
        {
            return;
        }
        process_detail::fail("terminate_failed", "强制结束子进程失败", error);
    }
    child->killed = true;
}

void process_close(const process_child& child)
{
    if (child)
    {
        child->close();
    }
}

} // namespace tx_generated
