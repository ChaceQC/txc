#include "backend/cpp/error_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/closure.hpp"
#include "stdlib/task.hpp"

#include <any>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

extern "C" int txrt_thread_spawn(const void*, std::int64_t, void**) noexcept;
extern "C" int txrt_thread_join_str(const void*, void**) noexcept;
extern "C" int txrt_thread_join_value(const void*, void**) noexcept;
extern "C" int txrt_task_wait_str(const void*, void**) noexcept;
extern "C" int txrt_task_wait_value(const void*, void**) noexcept;

namespace
{

using tx_generated::detail::runtime_context;

struct context_scope
{
    runtime_context context;
    runtime_context* previous = tx_generated::detail::thread_context;

    context_scope()
    {
        tx_generated::detail::thread_context = &context;
    }

    ~context_scope()
    {
        tx_generated::detail::thread_context = previous;
    }
};

void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

template<class target_type>
std::any make_callback(target_type target)
{
    tx_generated::closure_state state;
    state.target = reinterpret_cast<const void*>(target);
    return tx_generated::closure_handle(std::move(state));
}

void set_original_error()
{
    auto& context = tx_generated::detail::current_runtime_context();
    tx_generated::detail::diagnostic_frame frame;
    frame.source = {"original_callback", "concurrency_error_abi.cpp", 42, 3};
    context.active_frame = &frame;
    tx_generated::detail::set_error(tx::error_kind::io,
        "audit_original", "原始跨线程错误");
    context.active_frame = nullptr;
}

std::string* null_text(const void*)
{
    return nullptr;
}

std::any* null_value(const void*)
{
    return nullptr;
}

std::string* original_text_then_throw(const void*)
{
    set_original_error();
    throw std::runtime_error("后续 C++ 异常");
}

std::any* original_value_then_throw(const void*)
{
    set_original_error();
    throw 7;
}

std::string* good_text(const void*)
{
    return tx_generated::detail::make_handle<std::string>("正常结果");
}

std::string* nested_wait_after_error(const void*)
{
    auto scope = tx_generated::current_task_scope();
    auto child = tx_generated::submit_task(scope,
        make_callback(&original_text_then_throw), 4);
    // 工作线程等待子任务时会进入帮助执行路径，子任务上下文必须独立。
    tx_generated::wait_task(child);
    {
        std::lock_guard lock(child->mutex);
        require(child->error.kind == tx::error_kind::io &&
            child->error.code == "audit_original", "子任务错误快照丢失");
    }
    require(tx_generated::detail::current_runtime_context().last_error_kind ==
        tx::error_kind::none, "子任务错误泄漏到等待者上下文");
    return tx_generated::detail::make_handle<std::string>("嵌套等待成功");
}

void check_error(const runtime_context& context, bool original,
                 bool thread, bool text)
{
    require(context.last_error_kind == (original
        ? tx::error_kind::io : tx::error_kind::runtime), "错误类别不符");
    const auto code = original ? "audit_original"
        : thread ? "thread_failed" : "task_failed";
    require(std::string(context.last_error_code) == code, "错误码不符");
    const auto message = original ? "原始跨线程错误"
        : text ? "跨线程文本结果为空" : "跨线程复合结果为空";
    require(std::string(context.last_error) == message, "错误消息不符");
    require(context.last_error_stack.size() == (original ? 1u : 0u),
        "错误栈帧数不符");
    if (original)
    {
        const auto& frame = context.last_error_stack[0];
        require(std::string(frame.function) == "original_callback" &&
            std::string(frame.file) == "concurrency_error_abi.cpp" &&
            frame.line == 42 && frame.column == 3, "原始错误栈被覆盖");
    }
}

template<class target_type>
void check_thread(target_type target, bool original, bool text)
{
    context_scope local;
    auto callback = make_callback(target);
    void* handle = nullptr;
    require(txrt_thread_spawn(&callback, text ? 4 : 5, &handle) == 0,
        "无法启动线程");
    void* result = nullptr;
    const auto status = text ? txrt_thread_join_str(handle, &result)
        : txrt_thread_join_value(handle, &result);
    require(status != 0 && result == nullptr, "线程没有交付错误");
    check_error(local.context, original, true, text);
    tx_generated::detail::destroy_handle(static_cast<std::any*>(handle));
}

template<class target_type>
void check_task(target_type target, bool original, bool text)
{
    context_scope local;
    auto scope = std::make_shared<tx_generated::task_scope_state>();
    scope->cancellation = std::make_shared<tx_generated::cancellation_state>();
    scope->maximum = 1;
    auto state = tx_generated::submit_task(scope, make_callback(target),
        text ? 4 : 5);
    std::any handle = tx_generated::task_handle{state, text ? 4 : 5,
        text ? "task<str>" : "task<any>"};
    void* result = nullptr;
    const auto status = text ? txrt_task_wait_str(&handle, &result)
        : txrt_task_wait_value(&handle, &result);
    require(status != 0 && result == nullptr, "任务没有交付错误");
    check_error(local.context, original, false, text);
}

void check_clean_task_after_error()
{
    context_scope local;
    auto scope = std::make_shared<tx_generated::task_scope_state>();
    scope->cancellation = std::make_shared<tx_generated::cancellation_state>();
    scope->maximum = 1;
    auto state = tx_generated::submit_task(scope, make_callback(&good_text), 4);
    tx_generated::wait_task(state);
    std::lock_guard lock(state->mutex);
    require(state->error.kind == tx::error_kind::none,
        "后续工作线程继承了旧错误");
    require(std::get<std::string>(state->result) == "正常结果",
        "后续任务结果错误");
}

void check_nested_wait_boundary()
{
    context_scope local;
    auto scope = std::make_shared<tx_generated::task_scope_state>();
    scope->cancellation = std::make_shared<tx_generated::cancellation_state>();
    scope->maximum = 2;
    auto state = tx_generated::submit_task(scope,
        make_callback(&nested_wait_after_error), 4);
    tx_generated::wait_task(state);
    std::lock_guard lock(state->mutex);
    require(state->error.kind == tx::error_kind::none,
        "嵌套等待污染父任务错误");
    require(std::get<std::string>(state->result) == "嵌套等待成功",
        "嵌套等待结果错误");
}

} // namespace

int main()
{
    try
    {
        check_thread(&null_text, false, true);
        check_thread(&null_value, false, false);
        check_task(&null_text, false, true);
        check_task(&null_value, false, false);
        check_thread(&original_text_then_throw, true, true);
        check_thread(&original_value_then_throw, true, false);
        check_task(&original_text_then_throw, true, true);
        check_task(&original_value_then_throw, true, false);
        check_nested_wait_boundary();
        check_clean_task_after_error();
        std::cout << "CONCURRENCY_ERROR_ABI_OK\n";
        return 0;
    }
    catch (const std::exception& failure)
    {
        std::cerr << failure.what() << '\n';
        return 1;
    }
}
