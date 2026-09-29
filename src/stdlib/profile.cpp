#include "stdlib/profile_internal.hpp"
#include "stdlib/error.hpp"

namespace tx_generated::profiling
{

thread_local bool inside_profiler = false;

profile_state& state()
{
    static auto* current = new profile_state();
    return *current;
}

} // namespace tx_generated::profiling

namespace tx_generated
{

void profile_register_thread(detail::runtime_context** context) noexcept
{
    profiling::internal_guard guard;
    try
    {
        auto& current = profiling::state();
        std::lock_guard lock(current.mutex);
        const auto id = GetCurrentThreadId();
        if (current.threads.contains(id))
        {
            return;
        }
        HANDLE handle = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT |
            THREAD_QUERY_INFORMATION, FALSE, id);
        if (handle)
        {
            current.threads.emplace(id, profiling::sampled_thread{handle, context, 0});
        }
    }
    catch (...)
    {
        // 诊断设施不能改变运行时初始化的错误语义。
    }
}

void profile_unregister_thread() noexcept
{
    profiling::internal_guard guard;
    auto& current = profiling::state();
    std::lock_guard lock(current.mutex);
    const auto found = current.threads.find(GetCurrentThreadId());
    if (found != current.threads.end())
    {
        CloseHandle(found->second.handle);
        current.threads.erase(found);
    }
}

void profile_start(std::int64_t interval_ms, std::int64_t maximum)
{
    profiling::internal_guard guard;
    if (interval_ms < 1 || interval_ms > 1000 || maximum < 1 || maximum > 1000000)
    {
        throw runtime_failure({tx::error_kind::runtime, "invalid_profile_options",
            "采样间隔必须为 1..1000 毫秒，分配跟踪上限必须为 1..1000000"});
    }
    profile_link_allocator();
    auto& current = profiling::state();
    std::lock_guard control(current.control);
    std::lock_guard lock(current.mutex);
    if (current.enabled.load())
    {
        throw runtime_failure({tx::error_kind::runtime, "profile_active", "性能分析已经启动"});
    }
    current.allocations.clear();
    current.samples.clear();
    current.spans.clear();
    current.allocated_bytes = current.allocation_count = current.dropped_allocations = 0;
    current.missed_samples = current.sampler_us = 0;
    current.elapsed_us = 0;
    current.maximum = static_cast<std::size_t>(maximum);
    current.interval_ms = interval_ms;
    for (auto& [id, thread] : current.threads)
    {
        (void)id;
        FILETIME created{}, exited{}, kernel{}, user{};
        GetThreadTimes(thread.handle, &created, &exited, &kernel, &user);
        current.start = profiling::clock_type::now();
        thread.cpu = (static_cast<std::uint64_t>(kernel.dwHighDateTime) << 32) +
            kernel.dwLowDateTime + (static_cast<std::uint64_t>(user.dwHighDateTime) << 32) +
            user.dwLowDateTime;
    }
    current.start = profiling::clock_type::now();
    current.sampler = std::jthread(profiling::sample_loop);
    current.enabled.store(true);
}

std::string profile_snapshot(bool stop)
{
    profiling::internal_guard guard;
    auto& current = profiling::state();
    std::lock_guard control(current.control);
    const bool was_active = current.enabled.load();
    if (stop)
    {
        current.enabled.store(false);
        if (current.sampler.joinable())
        {
            current.sampler.request_stop();
            current.sampler.join();
        }
    }
    std::lock_guard lock(current.mutex);
    if (!current.maximum)
    {
        throw runtime_failure({tx::error_kind::runtime, "profile_inactive", "性能分析尚未启动"});
    }
    if (was_active || !current.elapsed_us)
    {
        current.elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(
            profiling::clock_type::now() - current.start).count();
    }
    return profiling::report_locked(current);
}

std::int64_t profile_begin_span(const std::string& name)
{
    profiling::internal_guard guard;
    auto& current = profiling::state();
    std::lock_guard lock(current.mutex);
    if (!current.enabled || name.empty() || current.spans.size() >= 10000)
    {
        throw runtime_failure({tx::error_kind::runtime, "invalid_profile_span",
            "分段计时需要活动分析会话、非空名称且不超过 10000 段"});
    }
    const auto id = current.next_span++;
    current.spans.emplace(id, profiling::span{name, profiling::clock_type::now(), -1});
    return id;
}

std::int64_t profile_end_span(std::int64_t id)
{
    profiling::internal_guard guard;
    auto& current = profiling::state();
    std::lock_guard lock(current.mutex);
    const auto found = current.spans.find(id);
    if (!current.enabled || found == current.spans.end() || found->second.duration_us >= 0)
    {
        throw runtime_failure({tx::error_kind::runtime, "invalid_profile_span", "计时段不存在或已经结束"});
    }
    found->second.duration_us = std::chrono::duration_cast<std::chrono::microseconds>(
        profiling::clock_type::now() - found->second.start).count();
    return found->second.duration_us;
}

void profile_allocated(void* pointer, std::size_t bytes) noexcept
{
    if (profiling::inside_profiler)
    {
        return;
    }
    profiling::internal_guard guard;
    try
    {
        auto& current = profiling::state();
        if (!current.enabled.load(std::memory_order_relaxed))
        {
            return;
        }
        std::lock_guard lock(current.mutex);
        ++current.allocation_count;
        current.allocated_bytes += bytes;
        if (current.allocations.size() >= current.maximum)
        {
            ++current.dropped_allocations;
            return;
        }
        detail::source_frame source;
        if (detail::thread_context && detail::thread_context->active_frame)
        {
            source = detail::thread_context->active_frame->source;
        }
        current.allocations.insert_or_assign(pointer, profiling::allocation{bytes, source});
    }
    catch (...)
    {
        // 分配跟踪失败不让被分析程序的成功分配变成失败。
    }
}

void profile_freed(void* pointer) noexcept
{
    if (profiling::inside_profiler)
    {
        return;
    }
    profiling::internal_guard guard;
    auto& current = profiling::state();
    if (current.enabled.load(std::memory_order_relaxed))
    {
        std::lock_guard lock(current.mutex);
        current.allocations.erase(pointer);
    }
}

} // namespace tx_generated
