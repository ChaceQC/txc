#include "stdlib/stdlib.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
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

std::mt19937_64& generator()
{
    auto& current = state();
    if (!current.initialized)
    {
        std::random_device entropy;
        std::seed_seq seeds{entropy(), entropy(), entropy(), entropy()};
        current.engine.seed(seeds);
        current.initialized = true;
    }
    return current.engine;
}

} // namespace

void tx_fn_seed(tx_int value)
{
    auto& current = state();
    current.engine.seed(static_cast<std::uint64_t>(value));
    current.initialized = true;
}

tx_int tx_fn_random_int(tx_int lower, tx_int upper)
{
    if (lower > upper)
    {
        throw std::runtime_error("random_int 的下界不能大于上界");
    }
    std::uniform_int_distribution<tx_int> distribution(lower, upper);
    return distribution(generator());
}

double tx_fn_random_float()
{
    std::uniform_real_distribution<double> distribution(0.0, 1.0);
    const double value = distribution(generator());
    return std::min(value, std::nextafter(1.0, 0.0));
}

} // namespace tx_generated
