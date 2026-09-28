#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/runtime_abi.hpp"
#include "backend/cpp/runtime_context.hpp"
#include "backend/cpp/cycle_gc.hpp"
#include "backend/cpp/concurrency_result.hpp"
#include "stdlib/closure.hpp"

#include <any>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <variant>

namespace
{

using tx_generated::detail::runtime_context;

struct thread_error
{
    tx::error_kind kind = tx::error_kind::none;
    std::string code;
    std::string message;
    std::vector<tx_generated::detail::source_frame> stack;
};

struct thread_state
{
    std::mutex mutex;
    tx_generated::concurrent_result result;
    thread_error error;
    bool completed = false;
    bool detached = false;
    bool error_reported = false;
};

struct detached_error_queue
{
    std::mutex mutex;
    std::deque<std::string> errors;
};

detached_error_queue& detached_queue()
{
    // detached 线程可能晚于 main 返回，不析构进程级错误回收队列。
    static auto* queue = new detached_error_queue();
    return *queue;
}

std::string describe_error(const thread_error& error)
{
    return error.code + ": " + error.message;
}

void report_detached_error(const std::shared_ptr<thread_state>& state) noexcept
{
    try
    {
        std::string message;
        {
            std::lock_guard lock(state->mutex);
            if (!state->completed || !state->detached ||
                state->error.kind == tx::error_kind::none ||
                state->error_reported)
            {
                return;
            }
            state->error_reported = true;
            message = describe_error(state->error);
        }
        auto& queue = detached_queue();
        std::lock_guard lock(queue.mutex);
        queue.errors.push_back(std::move(message));
    }
    catch (...)
    {
        // 错误回收本身失败时不能从线程退出或析构路径抛异常。
    }
}

struct thread_control
{
    std::shared_ptr<thread_state> state;
    std::thread worker;
    std::mutex mutex;
    bool resolved = false;

    ~thread_control()
    {
        if (worker.joinable())
        {
            {
                std::lock_guard lock(state->mutex);
                state->detached = true;
            }
            worker.detach();
            report_detached_error(state);
        }
    }
};

struct thread_handle
{
    std::shared_ptr<thread_control> control;
};

std::shared_ptr<thread_control> require_handle(const void* value)
{
    if (!value)
    {
        throw std::runtime_error("线程句柄为空");
    }
    const auto& handle = std::any_cast<const thread_handle&>(
        *static_cast<const std::any*>(value));
    if (!handle.control)
    {
        throw std::runtime_error("线程句柄无效");
    }
    return handle.control;
}

thread_error snapshot_error(const runtime_context& context)
{
    return {context.last_error_kind, context.last_error_code,
            context.last_error, context.last_error_stack};
}

void run_worker(std::shared_ptr<thread_state> state, std::any callback,
                std::int64_t kind) noexcept
{
    auto& context = tx_generated::detail::current_runtime_context();
    context.propagate_errors = true;
    tx_generated::concurrent_result result;
    thread_error error;
    {
        tx_generated::concurrent_execution_scope execution;
        try
        {
            const auto& closure = std::any_cast<const tx_generated::closure_handle&>(
                callback);
            const auto target = closure.data().target;
            result = tx_generated::invoke_concurrent_callback(
                target, &callback, kind);
        }
        catch (const std::exception& error)
        {
            tx_generated::detail::set_error(tx::error_kind::runtime,
                "thread_failed", error.what());
        }
        catch (...)
        {
            tx_generated::detail::set_error(tx::error_kind::runtime,
                "thread_failed", "线程发生未知错误");
        }
        error = snapshot_error(context);
        callback.reset();
        tx_generated::detail::cleanup_live_handles();
        try
        {
            // 工作线程只扫描自己创建的节点；跨线程对象图留给 join 后的主线程。
            tx_generated::collect_cycles();
        }
        catch (...)
        {
            if (error.kind == tx::error_kind::none)
            {
                error = {tx::error_kind::runtime, "gc_failed",
                         "线程退出时循环回收失败", {}};
            }
        }
    }
    {
        std::lock_guard lock(state->mutex);
        state->result = result;
        state->error = std::move(error);
        state->completed = true;
    }
    report_detached_error(state);
}

void replay_error(const thread_error& error)
{
    tx_generated::detail::set_error(error.kind, error.code.c_str(),
                                    error.message.c_str());
    tx_generated::detail::current_runtime_context().last_error_stack =
        error.stack;
}

template<class value_type, class output_type>
int join_value(const void* value, output_type* result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        if (tx_generated::detail::current_runtime_context().collecting)
        {
            throw tx_generated::runtime_failure({tx::error_kind::runtime,
                "gc_in_progress", "循环回收期间不能等待线程"});
        }
        auto control = require_handle(value);
        {
            std::lock_guard lock(control->mutex);
            if (control->resolved)
            {
                throw tx_generated::runtime_failure({tx::error_kind::runtime,
                    "invalid_state", "线程已经 join 或 detach"});
            }
            control->resolved = true;
            control->worker.join();
        }
        tx_generated::collect_cycles();
        std::lock_guard lock(control->state->mutex);
        if (control->state->error.kind != tx::error_kind::none)
        {
            replay_error(control->state->error);
            return;
        }
        if constexpr (std::is_same_v<value_type, std::string>)
        {
            *result = tx_generated::detail::make_handle<std::string>(
                std::get<std::string>(control->state->result));
        }
        else if constexpr (std::is_same_v<value_type, std::any>)
        {
            *result = tx_generated::detail::make_handle<std::any>(
                std::get<std::any>(control->state->result));
        }
        else if constexpr (!std::is_same_v<value_type, std::monostate>)
        {
            *result = std::get<value_type>(control->state->result);
        }
    });
}

} // namespace

extern "C" int txrt_thread_spawn(const void* callback, std::int64_t kind,
                                    void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        if (tx_generated::detail::current_runtime_context().collecting)
        {
            throw tx_generated::runtime_failure({tx::error_kind::runtime,
                "gc_in_progress", "循环回收期间不能启动线程"});
        }
        if (!callback || kind < 0 || kind > 5)
        {
            throw tx_generated::runtime_failure({tx::error_kind::runtime,
                "invalid_argument", "线程回调或返回类型无效"});
        }
        std::any copied = *static_cast<const std::any*>(callback);
        (void)std::any_cast<const tx_generated::closure_handle&>(copied);
        auto control = std::make_shared<thread_control>();
        control->state = std::make_shared<thread_state>();
        control->worker = std::thread(run_worker, control->state,
                                      std::move(copied), kind);
        *result = tx_generated::detail::make_handle<std::any>(
            thread_handle{std::move(control)});
    });
}

extern "C" int txrt_thread_join_void(const void* value) noexcept
{
    return join_value<std::monostate>(value, static_cast<void**>(nullptr));
}

extern "C" int txrt_thread_join_i64(const void* value,
                                      std::int64_t* result) noexcept
{
    return join_value<std::int64_t>(value, result);
}

extern "C" int txrt_thread_join_f64(const void* value,
                                      double* result) noexcept
{
    return join_value<double>(value, result);
}

extern "C" int txrt_thread_join_bool(const void* value,
                                       bool* result) noexcept
{
    return join_value<bool>(value, result);
}

extern "C" int txrt_thread_join_str(const void* value,
                                      void** result) noexcept
{
    return join_value<std::string>(value, result);
}

extern "C" int txrt_thread_join_value(const void* value,
                                        void** result) noexcept
{
    return join_value<std::any>(value, result);
}

extern "C" int txrt_thread_detach(const void* value) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        auto control = require_handle(value);
        std::lock_guard lock(control->mutex);
        if (control->resolved)
        {
            throw tx_generated::runtime_failure({tx::error_kind::runtime,
                "invalid_state", "线程已经 join 或 detach"});
        }
        control->resolved = true;
        {
            std::lock_guard state_lock(control->state->mutex);
            control->state->detached = true;
        }
        control->worker.detach();
        report_detached_error(control->state);
    });
}

extern "C" int txrt_thread_id(const void* value,
                                 std::int64_t* result) noexcept
{
    return tx_generated::detail::invoke_leaf([&]
    {
        auto control = require_handle(value);
        std::lock_guard lock(control->mutex);
        if (control->resolved)
        {
            throw tx_generated::runtime_failure({tx::error_kind::runtime,
                "invalid_state", "线程已经 join 或 detach"});
        }
        *result = static_cast<std::int64_t>(
            std::hash<std::thread::id>{}(control->worker.get_id()));
    });
}

extern "C" int txrt_thread_take_detached_error(void** result) noexcept
{
    return tx_generated::detail::invoke_leaf([&]
    {
        std::string message;
        {
            auto& queue = detached_queue();
            std::lock_guard lock(queue.mutex);
            if (!queue.errors.empty())
            {
                message = std::move(queue.errors.front());
                queue.errors.pop_front();
            }
        }
        *result = tx_generated::detail::make_handle<std::string>(
            std::move(message));
    });
}
