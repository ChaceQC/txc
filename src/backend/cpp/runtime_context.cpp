#include "backend/cpp/runtime_context.hpp"

#include <algorithm>

namespace tx_generated::detail
{

constinit thread_local runtime_context* thread_context = nullptr;

runtime_context& initialize_runtime_context() noexcept
{
    // 真正的所有者只在首次访问时初始化，仍由线程退出时的析构负责释放。
    static thread_local runtime_context context;
    thread_context = &context;
    return context;
}

std::vector<source_frame> capture_stack(const runtime_context& context)
{
    std::vector<source_frame> result;
    for (auto* frame = context.active_frame; frame; frame = frame->parent)
    {
        result.push_back(frame->source);
    }
    std::reverse(result.begin(), result.end());
    return result;
}

} // namespace tx_generated::detail

extern "C" void* txrt_runtime_context() noexcept
{
    return &tx_generated::detail::current_runtime_context();
}
