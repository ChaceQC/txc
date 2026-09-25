#include "backend/cpp/numeric_abi.hpp"
#include "backend/cpp/runtime_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/stdlib.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>

namespace tx_generated
{
namespace
{

struct random_state
{
    std::mt19937_64 engine;
    bool initialized = false;
};

random_state& state()
{
    // TX 暂无线程 API；把生成器状态限制在调用线程内，避免全局共享状态。
    static thread_local random_state value;
    return value;
}

std::mt19937_64& generator(random_state& current)
{
    if (!current.initialized)
    {
        std::random_device entropy;
        std::seed_seq seeds{entropy(), entropy(), entropy(), entropy()};
        current.engine.seed(seeds);
        current.initialized = true;
    }
    return current.engine;
}

void seed_state(random_state& current, tx_int value)
{
    current.engine.seed(static_cast<std::uint64_t>(value));
    current.initialized = true;
}

tx_int generate_int(random_state& current, tx_int lower, tx_int upper)
{
    if (lower > upper)
    {
        throw std::runtime_error("random_int 的下界不能大于上界");
    }
    std::uniform_int_distribution<tx_int> distribution(lower, upper);
    return distribution(generator(current));
}

double generate_float(random_state& current)
{
    std::uniform_real_distribution<double> distribution(0.0, 1.0);
    const double value = distribution(generator(current));
    return std::min(value, std::nextafter(1.0, 0.0));
}

} // namespace

void tx_fn_seed(tx_int value)
{
    seed_state(state(), value);
}

tx_int tx_fn_random_int(tx_int lower, tx_int upper)
{
    return generate_int(state(), lower, upper);
}

double tx_fn_random_float()
{
    return generate_float(state());
}

void* tx_random_context()
{
    return &state();
}

void tx_seed_context(void* context, tx_int value)
{
    seed_state(*static_cast<random_state*>(context), value);
}

tx_int tx_random_int_context(void* context, tx_int lower, tx_int upper)
{
    return generate_int(*static_cast<random_state*>(context), lower, upper);
}

double tx_random_float_context(void* context)
{
    return generate_float(*static_cast<random_state*>(context));
}

} // namespace tx_generated

using tx_generated::detail::invoke_checked;

namespace
{

template<class operation>
auto invoke_direct(operation&& run) noexcept -> decltype(run())
{
    decltype(run()) result{};
    txrt_require_success(invoke_checked([&]
    {
        result = run();
    }));
    return result;
}

} // namespace

extern "C" int txrt_random_seed_i64(std::int64_t value) noexcept
{
    return invoke_checked([&] { tx_generated::tx_fn_seed(value); });
}

extern "C" int txrt_random_int_i64(std::int64_t lower, std::int64_t upper,
                                    std::int64_t* result) noexcept
{
    return invoke_checked([&] {
        *result = tx_generated::tx_fn_random_int(lower, upper);
    });
}

extern "C" int txrt_random_float_f64(double* result) noexcept
{
    return invoke_checked([&] { *result = tx_generated::tx_fn_random_float(); });
}

extern "C" std::int64_t txrt_random_int_direct(std::int64_t lower,
                                                 std::int64_t upper) noexcept
{
    return invoke_direct([&] {
        return tx_generated::tx_fn_random_int(lower, upper);
    });
}

extern "C" double txrt_random_float_direct() noexcept
{
    return invoke_direct([] { return tx_generated::tx_fn_random_float(); });
}

extern "C" void* txrt_random_context() noexcept
{
    return invoke_direct([] { return tx_generated::tx_random_context(); });
}

extern "C" int txrt_random_seed_context(void* context,
                                          std::int64_t value) noexcept
{
    return invoke_checked([&] {
        tx_generated::tx_seed_context(context, value);
    });
}

extern "C" std::int64_t txrt_random_int_context(
    void* context, std::int64_t lower, std::int64_t upper) noexcept
{
    return invoke_direct([&] {
        return tx_generated::tx_random_int_context(context, lower, upper);
    });
}

extern "C" double txrt_random_float_context(void* context) noexcept
{
    return invoke_direct([&] {
        return tx_generated::tx_random_float_context(context);
    });
}
