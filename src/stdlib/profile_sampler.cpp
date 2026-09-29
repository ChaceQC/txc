#include "stdlib/profile_internal.hpp"

#include <array>

namespace tx_generated::profiling
{
namespace
{

template<class value_type>
bool read_memory(const value_type* address, value_type& value)
{
    SIZE_T read = 0;
    return address && ReadProcessMemory(GetCurrentProcess(), address, &value,
        sizeof(value), &read) && read == sizeof(value);
}

struct sampled_location
{
    detail::source_frame source;
    const char* abi = "";
    std::uint64_t instruction = 0;
    std::uint64_t cpu = 0;
};

bool read_sample(sampled_thread& thread, sampled_location& sample)
{
    FILETIME created{}, exited{}, kernel{}, user{};
    if (!GetThreadTimes(thread.handle, &created, &exited, &kernel, &user))
    {
        return false;
    }
    const auto cpu = (static_cast<std::uint64_t>(kernel.dwHighDateTime) << 32) +
        kernel.dwLowDateTime + (static_cast<std::uint64_t>(user.dwHighDateTime) << 32) +
        user.dwLowDateTime;
    sample.cpu = cpu >= thread.cpu ? cpu - thread.cpu : 0;
    thread.cpu = cpu;
    if (!sample.cpu)
    {
        return true;
    }
    if (SuspendThread(thread.handle) == static_cast<DWORD>(-1))
    {
        return false;
    }
    // 暂停期间只能使用栈缓冲和系统读取，不能分配内存或等待被采样线程的锁。
    CONTEXT native{};
    native.ContextFlags = CONTEXT_CONTROL;
    const bool native_ok = GetThreadContext(thread.handle, &native);
    detail::runtime_context* context = nullptr;
    detail::diagnostic_frame* frame = nullptr;
    detail::diagnostic_frame* abi_frame = nullptr;
    const char* abi = nullptr;
    if (read_memory(thread.context, context) && context &&
        read_memory(&context->active_frame, frame) && frame)
    {
        read_memory(&frame->source, sample.source);
        if (read_memory(&context->profile_abi_frame, abi_frame) && abi_frame == frame &&
            read_memory(&context->profile_abi, abi) && abi)
        {
            sample.abi = abi;
        }
    }
    sample.instruction = native.Rip;
    ResumeThread(thread.handle);
    return native_ok;
}

void sample_once(profile_state& current)
{
    std::lock_guard lock(current.mutex);
    for (auto& [id, thread] : current.threads)
    {
        (void)id;
        sampled_location location;
        if (!read_sample(thread, location))
        {
            ++current.missed_samples;
            continue;
        }
        if (!location.cpu)
        {
            continue;
        }
        const auto& frame = location.source;
        const auto key = std::string(frame.file) + ":" + std::to_string(frame.line) +
            ":" + frame.function + ":" + location.abi;
        if (current.samples.size() >= 100000 && !current.samples.contains(key))
        {
            ++current.missed_samples;
            continue;
        }
        auto& sample = current.samples[key];
        sample.source = frame;
        sample.abi = location.abi;
        sample.instruction = location.instruction;
        sample.cpu_100ns += location.cpu;
        ++sample.count;
    }
}

} // namespace

void sample_loop(std::stop_token token)
{
    internal_guard guard;
    auto& current = state();
    while (!token.stop_requested())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(current.interval_ms));
        const auto start = clock_type::now();
        try
        {
            sample_once(current);
        }
        catch (...)
        {
            std::lock_guard lock(current.mutex);
            ++current.missed_samples;
        }
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            clock_type::now() - start).count();
        std::lock_guard lock(current.mutex);
        current.sampler_us += elapsed;
    }
}

} // namespace tx_generated::profiling
