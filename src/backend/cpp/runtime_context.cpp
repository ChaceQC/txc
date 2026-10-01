#include "backend/cpp/runtime_context.hpp"
#include "stdlib/profile.hpp"

#include <algorithm>
#include <exception>
#include <memory>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace tx_generated::detail
{

constinit thread_local runtime_context* thread_context = nullptr;

#ifdef _WIN32
namespace
{

void NTAPI release_runtime_context(void* value) noexcept
{
    static_cast<runtime_context*>(value)->close_graphics();
    profile_unregister_thread();
    thread_context = nullptr;
    delete static_cast<runtime_context*>(value);
}

DWORD context_slot() noexcept
{
    static const DWORD slot = FlsAlloc(&release_runtime_context);
    return slot;
}

} // namespace
#endif

runtime_context& initialize_runtime_context() noexcept
{
#ifdef _WIN32
    // 非平凡 C++ thread_local 对象在当前 MinGW 线程退出路径中可能被重复析构。
    // FLS 回调直接持有堆对象，不依赖 C++ 的线程析构登记表。
    const DWORD slot = context_slot();
    if (slot == FLS_OUT_OF_INDEXES)
    {
        std::terminate();
    }
    auto* context = static_cast<runtime_context*>(FlsGetValue(slot));
    if (!context)
    {
        auto pending = std::make_unique<runtime_context>();
        if (!FlsSetValue(slot, pending.get()))
        {
            std::terminate();
        }
        context = pending.release();
    }
    thread_context = context;
    profile_register_thread(&thread_context);
    return *context;
#else
    static thread_local runtime_context context;
    thread_context = &context;
    return context;
#endif
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
