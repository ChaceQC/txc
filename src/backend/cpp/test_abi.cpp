#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/closure.hpp"
#include "stdlib/json.hpp"
#include "stdlib/stdlib.hpp"
#include "stdlib/test.hpp"

#include <any>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace
{

using tx_generated::detail::invoke_checked;
std::atomic<std::int64_t> registered_failures = 0;

const std::string& text(const void* value)
{
    return tx_generated::detail::text_value(value);
}

[[noreturn]] void fail(const std::string& detail, const void* message)
{
    const auto& label = text(message);
    throw tx_generated::runtime_failure({tx::error_kind::runtime,
        "assertion_failed", "断言失败：" + detail +
        (label.empty() ? "" : "（" + label + "）")});
}

template<class value_type>
void assert_equal(value_type expected, value_type actual, const void* message)
{
    if (expected != actual)
    {
        fail("期望 " + tx_generated::tx_to_string(std::any(expected)) +
             "，实际 " + tx_generated::tx_to_string(std::any(actual)), message);
    }
}

struct callback_result
{
    bool raised = false;
    std::string code;
    std::string message;
    std::vector<tx_generated::detail::source_frame> stack;
};

callback_result invoke_operation(const void* operation)
{
    if (!operation)
    {
        throw std::runtime_error("预期异常回调为空");
    }
    const auto& closure = std::any_cast<const tx_generated::closure_handle&>(
        *static_cast<const std::any*>(operation));
    const auto& state = closure.data();
    if (!state.target)
    {
        throw std::runtime_error("预期异常回调没有调用目标");
    }
    using callback_type = void (*)(void*);
    auto& context = tx_generated::detail::current_runtime_context();
    const bool previous_propagation = context.propagate_errors;
    context.propagate_errors = true;
    try
    {
        reinterpret_cast<callback_type>(const_cast<void*>(state.target))(
            const_cast<void*>(operation));
    }
    catch (...)
    {
        context.propagate_errors = previous_propagation;
        throw;
    }
    context.propagate_errors = previous_propagation;
    callback_result result;
    result.raised = context.last_error_kind !=
        tx::error_kind::none;
    if (result.raised)
    {
        result.code = context.last_error_code;
        result.message = context.last_error;
        result.stack = std::move(context.last_error_stack);
    }
    // 预期错误在此边界被消费，不改变之后断言的错误状态。
    context.last_error_kind = tx::error_kind::none;
    context.last_error_stack.clear();
    return result;
}

void check_throws(const void* operation, const std::string* expected_code)
{
    const auto result = invoke_operation(operation);
    if (!result.raised)
    {
        throw tx_generated::runtime_failure({tx::error_kind::runtime,
            "assertion_failed", "断言失败：预期回调抛出 TX 错误"});
    }
    if (expected_code && result.code != *expected_code)
    {
        throw tx_generated::runtime_failure({tx::error_kind::runtime,
            "assertion_failed", "断言失败：预期错误代码 " + *expected_code +
            "，实际 " + result.code});
    }
}

bool run_case(const void* name_value, const void* operation)
{
    const auto& name = text(name_value);
    if (name.empty())
    {
        throw tx_generated::runtime_failure({tx::error_kind::runtime,
            "invalid_name", "测试项名称不能为空"});
    }
    const auto result = invoke_operation(operation);
    const auto quoted = tx_generated::json_stringify(std::any(name));
    if (!result.raised)
    {
        tx_generated::tx_fn_write_error("测试通过 " + quoted + "\n");
        return true;
    }
    ++registered_failures;
    std::string report = "测试失败 " + quoted + " [" + result.code +
        "] " + result.message + "\n";
    for (const auto& frame : result.stack)
    {
        report += "  位于 ";
        report += frame.function;
        report += " (";
        report += frame.file;
        report += ":" + std::to_string(frame.line) + ":" +
            std::to_string(frame.column) + ")\n";
    }
    tx_generated::tx_fn_write_error(report);
    return false;
}

} // namespace

namespace tx_generated
{

void record_test_failure() noexcept
{
    ++registered_failures;
}

} // namespace tx_generated

extern "C" int txrt_test_assert_true(bool condition,
                                      const void* message) noexcept
{
    return invoke_checked([&]
    {
        if (!condition)
        {
            fail("条件应为 true", message);
        }
    });
}

extern "C" int txrt_test_assert_false(bool condition,
                                       const void* message) noexcept
{
    return invoke_checked([&]
    {
        if (condition)
        {
            fail("条件应为 false", message);
        }
    });
}

extern "C" int txrt_test_assert_equal_i64(std::int64_t expected,
    std::int64_t actual, const void* message) noexcept
{
    return invoke_checked([&] { assert_equal(expected, actual, message); });
}

extern "C" int txrt_test_assert_equal_f64(double expected, double actual,
                                             const void* message) noexcept
{
    return invoke_checked([&] { assert_equal(expected, actual, message); });
}

extern "C" int txrt_test_assert_equal_bool(bool expected, bool actual,
                                              const void* message) noexcept
{
    return invoke_checked([&] { assert_equal(expected, actual, message); });
}

extern "C" int txrt_test_assert_equal_str(const void* expected,
    const void* actual, const void* message) noexcept
{
    return invoke_checked([&]
    {
        assert_equal(text(expected), text(actual), message);
    });
}

extern "C" int txrt_test_assert_near(double expected, double actual,
    double tolerance, const void* message) noexcept
{
    return invoke_checked([&]
    {
        if (!std::isfinite(tolerance) || tolerance < 0.0)
        {
            throw tx_generated::runtime_failure({tx::error_kind::runtime,
                "invalid_tolerance", "近似比较误差必须是有限非负数"});
        }
        const bool equal = expected == actual ||
            (std::isfinite(expected) && std::isfinite(actual) &&
             std::fabs(expected - actual) <= tolerance);
        if (!equal)
        {
            fail("期望 " + tx_generated::tx_float_to_string(expected) +
                 "，实际 " + tx_generated::tx_float_to_string(actual) +
                 "，绝对误差 " + tx_generated::tx_float_to_string(tolerance),
                 message);
        }
    });
}

extern "C" int txrt_test_assert_throws(const void* operation) noexcept
{
    return invoke_checked([&] { check_throws(operation, nullptr); });
}

extern "C" int txrt_test_assert_throws_code(const void* operation,
    const void* code) noexcept
{
    return invoke_checked([&] { check_throws(operation, &text(code)); });
}

extern "C" int txrt_test_run_case(const void* name,
    const void* operation, bool* result) noexcept
{
    return invoke_checked([&] { *result = run_case(name, operation); });
}

extern "C" int txrt_test_failures(std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = registered_failures.load();
    });
}

extern "C" std::int64_t txrt_test_failure_total() noexcept
{
    return registered_failures.load();
}

extern "C" int txrt_test_temp_directory(void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::detail::make_handle<std::string>(
            tx_generated::create_test_directory());
    });
}

extern "C" int txrt_test_seed(std::int64_t value) noexcept
{
    return invoke_checked([&] { tx_generated::tx_fn_seed(value); });
}
