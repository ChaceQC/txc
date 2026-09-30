#include "stdlib/profile_internal.hpp"

#include <array>
#include <cstring>
#include <sys/uio.h>
#include <unistd.h>

namespace tx_generated::profiling
{
namespace
{

template<class value_type>
bool read_memory(const value_type* address, value_type& value)
{
    if (!address)
    {
        return false;
    }
    iovec local{&value, sizeof(value)};
    iovec remote{const_cast<value_type*>(address), sizeof(value)};
    return process_vm_readv(getpid(), &local, 1, &remote, 1, 0) == sizeof(value);
}

bool same_source(const detail::source_frame& left, const detail::source_frame& right)
{
    return left.file == right.file && left.function == right.function &&
        left.line == right.line && left.column == right.column;
}

bool read_text(const char* address, std::string& text)
{
    text.clear();
    for (std::size_t offset = 0; address && offset < 4096;)
    {
        std::array<char, 128> buffer;
        iovec local{buffer.data(), buffer.size()};
        iovec remote{const_cast<char*>(address + offset), buffer.size()};
        const auto count = process_vm_readv(getpid(), &local, 1, &remote, 1, 0);
        if (count <= 0)
        {
            return false;
        }
        const auto* end = static_cast<const char*>(std::memchr(buffer.data(), 0, count));
        text.append(buffer.data(), end ? end - buffer.data() : count);
        if (end)
        {
            return true;
        }
        offset += static_cast<std::size_t>(count);
    }
    return false;
}

bool read_source(sampled_thread& thread, detail::source_frame& source, const char*& abi)
{
    detail::runtime_context* context = nullptr;
    detail::diagnostic_frame* frame = nullptr;
    detail::diagnostic_frame* after = nullptr;
    detail::source_frame second;
    if (!read_memory(thread.context, context) || !context ||
        !read_memory(&context->active_frame, frame) || !frame ||
        !read_memory(&frame->source, source) ||
        !read_memory(&frame->source, second) ||
        !read_memory(&context->active_frame, after) || frame != after ||
        !same_source(source, second))
    {
        return false;
    }
    // 不暂停线程或安装进程信号处理器；系统拷贝失败及切换中的帧记为未归因采样。
    detail::diagnostic_frame* abi_frame = nullptr;
    const char* current_abi = nullptr;
    if (read_memory(&context->profile_abi_frame, abi_frame) && abi_frame == frame &&
        read_memory(&context->profile_abi, current_abi) && current_abi)
    {
        abi = current_abi;
    }
    return source.file && source.function;
}

void sample_once(profile_state& current)
{
    std::lock_guard lock(current.mutex);
    for (auto& [id, thread] : current.threads)
    {
        (void)id;
        timespec cpu{};
        if (clock_gettime(thread.cpu_clock, &cpu) != 0)
        {
            ++current.missed_samples;
            continue;
        }
        const auto total = static_cast<std::uint64_t>(cpu.tv_sec) * 10000000 + cpu.tv_nsec / 100;
        const auto elapsed = total >= thread.cpu ? total - thread.cpu : 0;
        thread.cpu = total;
        if (!elapsed)
        {
            continue;
        }
        detail::source_frame source;
        const char* abi = "";
        std::string file, function, abi_name;
        if (!read_source(thread, source, abi) || !read_text(source.file, file) ||
            !read_text(source.function, function) || !read_text(abi, abi_name))
        {
            source = {};
            file.clear();
            function.clear();
            abi_name.clear();
            ++current.missed_samples;
        }
        const auto key = file + ':' + std::to_string(source.line) + ':' + function + ':' + abi_name;
        if (current.samples.size() >= 100000 && !current.samples.contains(key))
        {
            ++current.missed_samples;
            continue;
        }
        auto& sample = current.samples[key];
        sample.source = source;
        sample.source_file = std::move(file);
        sample.source_function = std::move(function);
        sample.source.file = sample.source_file.c_str();
        sample.source.function = sample.source_function.c_str();
        sample.abi = std::move(abi_name);
        sample.cpu_100ns += elapsed;
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
        std::lock_guard lock(current.mutex);
        current.sampler_us += std::chrono::duration_cast<std::chrono::microseconds>(clock_type::now() - start).count();
    }
}

} // namespace tx_generated::profiling
