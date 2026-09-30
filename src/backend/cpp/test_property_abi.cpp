#include "backend/cpp/checked_callback.hpp"
#include "stdlib/json.hpp"
#include "stdlib/stdlib.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_set>

namespace tx_generated
{
void record_test_failure() noexcept;
}

namespace
{

using namespace tx_generated;
using detail::invoke_checked;

void validate(const std::string& name, std::int64_t count, std::int64_t minimum)
{
    if (name.empty() || count < minimum || count > 100000)
    {
        throw runtime_failure({tx::error_kind::runtime, "invalid_test_options",
            "测试名称不能为空，次数必须在允许范围内"});
    }
}

void report(tx_dict event)
{
    const auto& stack = detail::current_runtime_context().last_error_stack;
    if (!stack.empty())
    {
        const auto& frame = stack.back();
        event.emplace_back(std::string("file"), std::string(frame.file));
        event.emplace_back(std::string("line"), static_cast<std::int64_t>(frame.line));
    }
    record_test_failure();
    tx_fn_write_error(json_stringify(std::any(event)) + "\n");
    detail::current_runtime_context().last_error_stack.clear();
}

using predicate_callback = bound_typed_callback<bool, std::int64_t>;

bool predicate_holds(const void* predicate, std::optional<predicate_callback>& bound,
    std::int64_t value, std::string& code)
{
    try
    {
        if (!bound)
        {
            bound.emplace(predicate);
        }
        return (*bound)(value);
    }
    catch (const runtime_failure& error)
    {
        code = error.error().code;
        return false;
    }
}

bool check_property(const std::string& name, std::int64_t seed, std::int64_t count,
    const void* generate, const void* predicate, const void* shrink, std::int64_t maximum)
{
    validate(name, count, 1);
    if (maximum < 0 || maximum > 4096)
    {
        throw runtime_failure({tx::error_kind::runtime, "invalid_test_options",
            "反例缩减步数必须在 0 到 4096 之间"});
    }
    const bound_typed_callback<std::int64_t, std::int64_t, std::int64_t> generator(generate);
    std::optional<predicate_callback> bound_predicate;
    std::optional<bound_typed_callback<std::int64_t, std::int64_t>> bound_shrink;
    for (std::int64_t index = 0; index < count; ++index)
    {
        auto original = generator(seed, index);
        std::string code;
        if (predicate_holds(predicate, bound_predicate, original, code))
        {
            continue;
        }
        auto minimal = original;
        std::int64_t steps = 0;
        std::unordered_set<std::int64_t> visited{original};
        while (steps < maximum)
        {
            if (!bound_shrink)
            {
                bound_shrink.emplace(shrink);
            }
            const auto candidate = (*bound_shrink)(minimal);
            if (!visited.insert(candidate).second)
            {
                break;
            }
            ++steps;
            if (predicate_holds(predicate, bound_predicate, candidate, code))
            {
                break;
            }
            minimal = candidate;
        }
        tx_dict event;
        event.emplace_back(std::string("event"), std::string("property_failure"));
        event.emplace_back(std::string("name"), name);
        event.emplace_back(std::string("seed"), seed);
        event.emplace_back(std::string("index"), index);
        event.emplace_back(std::string("original"), original);
        event.emplace_back(std::string("counterexample"), minimal);
        event.emplace_back(std::string("shrink_steps"), steps);
        event.emplace_back(std::string("error_code"), code);
        report(std::move(event));
        return false;
    }
    return true;
}

} // namespace

extern "C" int txrt_test_parameterized(const void* name_value, std::int64_t count,
    const void* operation, bool* result) noexcept
{
    return invoke_checked([&]
    {
        const auto& name = detail::text_value(name_value);
        validate(name, count, 0);
        *result = true;
        std::optional<bound_typed_callback<void, std::int64_t>> bound;
        for (std::int64_t index = 0; index < count; ++index)
        {
            try
            {
                if (!bound)
                {
                    bound.emplace(operation);
                }
                (*bound)(index);
            }
            catch (const runtime_failure& error)
            {
                tx_dict event;
                event.emplace_back(std::string("event"), std::string("parameter_failure"));
                event.emplace_back(std::string("name"), name);
                event.emplace_back(std::string("index"), index);
                event.emplace_back(std::string("error_code"), error.error().code);
                report(std::move(event));
                *result = false;
            }
        }
    });
}

extern "C" int txrt_test_property(const void* name, std::int64_t seed,
    std::int64_t count, const void* generate, const void* predicate,
    const void* shrink, std::int64_t maximum, bool* result) noexcept
{
    return invoke_checked([&]
    {
        try
        {
            *result = check_property(detail::text_value(name), seed, count,
                                     generate, predicate, shrink, maximum);
        }
        catch (const runtime_failure& error)
        {
            if (error.error().code == "invalid_test_options")
            {
                throw;
            }
            tx_dict event;
            event.emplace_back(std::string("event"), std::string("property_callback_error"));
            event.emplace_back(std::string("name"), detail::text_value(name));
            event.emplace_back(std::string("seed"), seed);
            event.emplace_back(std::string("error_code"), error.error().code);
            report(std::move(event));
            *result = false;
        }
    });
}
