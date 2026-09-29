#include "backend/cpp/checked_callback.hpp"
#include "stdlib/json.hpp"
#include "stdlib/stdlib.hpp"

#include <limits>

namespace tx_generated
{
void record_test_failure() noexcept;
}

namespace
{

thread_local bool clock_fixed = false;
thread_local std::int64_t clock_value = 0;

bool run_phase(const std::string& name, const char* phase, const void* operation)
{
    try
    {
        tx_generated::invoke_typed_callback<void>(operation);
        return true;
    }
    catch (const tx_generated::runtime_failure& error)
    {
        tx_generated::record_test_failure();
        tx_generated::tx_fn_write_error("测试夹具失败 " +
            tx_generated::json_stringify(std::any(name)) + " [" + phase + "] " +
            error.error().code + "\n");
        for (const auto& frame : tx_generated::detail::current_runtime_context().last_error_stack)
        {
            tx_generated::tx_fn_write_error(std::string("  位于 ") + frame.function +
                " (" + frame.file + ":" + std::to_string(frame.line) + ":" +
                std::to_string(frame.column) + ")\n");
        }
        tx_generated::detail::current_runtime_context().last_error_stack.clear();
        return false;
    }
}

} // namespace

extern "C" int txrt_test_fixture(const void* name, const void* setup,
    const void* operation, const void* teardown, bool* result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        const auto& label = tx_generated::detail::text_value(name);
        if (label.empty())
        {
            throw tx_generated::runtime_failure({tx::error_kind::runtime,
                "invalid_name", "测试夹具名称不能为空"});
        }
        const auto ready = run_phase(label, "setup", setup);
        const auto passed = ready && run_phase(label, "operation", operation);
        // 初始化部分失败也要调用清理，夹具负责使清理可重复且识别部分状态。
        const auto cleaned = run_phase(label, "teardown", teardown);
        *result = passed && cleaned;
    });
}

extern "C" int txrt_test_set_time_millis(std::int64_t value) noexcept
{
    clock_value = value;
    clock_fixed = true;
    return 0;
}

extern "C" int txrt_test_advance_time_millis(std::int64_t delta) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        if (!clock_fixed || delta < 0 || clock_value > std::numeric_limits<std::int64_t>::max() - delta)
        {
            throw tx_generated::runtime_failure({tx::error_kind::runtime,
                "invalid_test_clock", "测试时钟尚未固定、推进量为负或结果溢出"});
        }
        clock_value += delta;
    });
}

extern "C" int txrt_test_now_millis(std::int64_t* result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = clock_fixed ? clock_value : tx_generated::tx_fn_unix_millis();
    });
}

extern "C" int txrt_test_reset_time() noexcept
{
    clock_fixed = false;
    return 0;
}
