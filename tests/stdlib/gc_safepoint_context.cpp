#include "backend/cpp/cycle_gc.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/runtime_context.hpp"

#include <iostream>
#include <stdexcept>

int main()
{
    using tx_generated::concurrent_execution_scope;
    using tx_generated::detail::runtime_context;

    runtime_context outer;
    runtime_context inner;
    auto* previous = tx_generated::detail::thread_context;
    tx_generated::detail::thread_context = &outer;

    // 显式入口必须读取传入上下文，即使线程当前安装的是另一个上下文。
    inner.last_error_kind = tx::error_kind::io;
    bool valid = txrt_gc_safepoint_context(&inner) ==
        static_cast<int>(tx::error_kind::io);
    valid &= inner.last_error_kind == tx::error_kind::io;
    inner.last_error_kind = tx::error_kind::none;
    const auto failed = tx_generated::detail::invoke_checked([]
    {
        throw std::runtime_error("GC 错误路径");
    }, tx::error_kind::runtime, &inner);
    valid &= failed != 0 &&
        inner.last_error_kind == tx::error_kind::runtime &&
        outer.last_error_kind == tx::error_kind::none;
    inner.last_error_kind = tx::error_kind::none;
    inner.allocations_since_collection = 64;
    valid &= txrt_gc_safepoint_context(&inner) == 0;

    {
        concurrent_execution_scope outer_scope;
        valid &= outer.concurrent_depth == 1;
        // 嵌套任务换上下文时继承执行门深度，异常退出后成对恢复。
        inner.concurrent_depth = outer.concurrent_depth;
        tx_generated::detail::thread_context = &inner;
        try
        {
            concurrent_execution_scope inner_scope;
            valid &= inner.concurrent_depth == 2;
            throw 1;
        }
        catch (int)
        {
            valid &= inner.concurrent_depth == 1;
        }
        tx_generated::detail::thread_context = &outer;
    }
    valid &= outer.concurrent_depth == 0;
    tx_generated::detail::thread_context = previous;

    if (!valid)
    {
        std::cerr << "GC 安全点上下文或执行门深度异常\n";
        return 1;
    }
    std::cout << "GC_SAFEPOINT_CONTEXT_OK\n";
    return 0;
}
