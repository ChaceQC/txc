#include "stdlib/process_internal.hpp"
#include "stdlib/error.hpp"

#include <cerrno>
#include <csignal>
#include <sys/wait.h>
#include <thread>

namespace tx_generated::process_detail
{

[[noreturn]] void fail(const char* code, const char* message, int error)
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
    return state->deadline && steady_clock::now() >= *state->deadline
        ? "deadline_exceeded" : "";
}

bool poll_exit(process_child_state& child)
{
    if (child.completed)
    {
        return true;
    }
    int status = 0;
    pid_t result;
    do
    {
        result = ::waitpid(child.id, &status, WNOHANG);
    } while (result < 0 && errno == EINTR);
    if (result == 0)
    {
        return false;
    }
    if (result < 0)
    {
        fail("wait_failed", "查询子进程退出状态失败", errno);
    }
    child.status = WIFSIGNALED(status)
        ? process_status{"terminated", 128 + WTERMSIG(status)}
        : process_status{"exited", WEXITSTATUS(status)};
    child.completed = true;
    return true;
}

} // namespace tx_generated::process_detail

namespace tx_generated
{

process_child_state::~process_child_state()
{
    close();
}

void process_child_state::close() noexcept
{
    if (closed)
    {
        return;
    }
    closed = true;
    for (const auto& pipe : pipes)
    {
        if (pipe)
        {
            pipe->handle.reset();
        }
    }
    if (!completed && id > 0)
    {
        // close 不终止子进程；独立回收等待避免遗留僵尸或阻塞调用方。
        try
        {
            std::thread([pid = id]
            {
                int status;
                while (::waitpid(pid, &status, 0) < 0 && errno == EINTR)
                {
                }
            }).detach();
        }
        catch (...)
        {
            int status;
            ::waitpid(id, &status, WNOHANG);
        }
    }
}

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
        if (process_detail::steady_clock::now() >= limit)
        {
            return {timeout_ms == 0 ? "running" : "timeout", 0};
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return child->status;
}

void process_terminate(const process_child& child)
{
    process_detail::require_child(child);
    if (!process_detail::poll_exit(*child) &&
        ::kill(child->group ? -child->id : child->id, SIGTERM) < 0 && errno != ESRCH)
    {
        process_detail::fail("terminate_failed", "协作终止子进程失败", errno);
    }
}

void process_kill(const process_child& child)
{
    process_detail::require_child(child);
    if (!process_detail::poll_exit(*child) && ::kill(child->id, SIGKILL) < 0 && errno != ESRCH)
    {
        process_detail::fail("terminate_failed", "强制结束子进程失败", errno);
    }
}

void process_close(const process_child& child)
{
    if (child)
    {
        child->close();
    }
}

} // namespace tx_generated
