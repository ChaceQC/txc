#include "compare.hpp"

#include <any>
#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{

std::int64_t add(std::int64_t left, std::int64_t right)
{
    return left + right;
}

double add(double left, double right)
{
    return left + right;
}

std::int64_t recursive_sum(std::int64_t value)
{
    if (value == 0)
    {
        return 0;
    }
    return value + recursive_sum(value - 1);
}

using dynamic_array = std::vector<std::any>;
using named_values = std::unordered_map<std::string, std::any>;

std::int64_t runtime_seed()
{
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
        now).count();
    return millis > 0 ? 1 : 2;
}

std::int64_t collect(std::int64_t first, dynamic_array args,
                     named_values kwargs)
{
    return first + static_cast<std::int64_t>(args.size()) +
        static_cast<std::int64_t>(kwargs.size()) +
        std::any_cast<std::int64_t>(args[0]) +
        std::any_cast<std::int64_t>(kwargs.at("bonus"));
}

} // namespace

void bench_scalar_control()
{
    std::int64_t checksum = 0;
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 500000; ++i)
    {
        if (i <= 250000 && i > 0)
        {
            checksum += i;
        }
        else
        {
            checksum -= i;
        }
    }
    report("scalar_control", started, checksum);
}

void bench_updates()
{
    std::int64_t checksum = 0;
    std::int64_t value = runtime_seed();
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 500000; ++i)
    {
        value += i;
        checksum += ++value;
        value -= value / 7;
        --value;
        if (value > 1000000)
        {
            value -= 1000000;
        }
    }
    report("updates", started, checksum);
}

void bench_while_logic()
{
    std::int64_t checksum = 0;
    std::int64_t index = 0;
    const auto started = clock_type::now();
    while (index < 500000)
    {
        ++index;
        if (index <= 250000 || (index > 500000 && index < 0))
        {
            checksum += index;
        }
        else
        {
            checksum -= index;
        }
    }
    report("while_logic", started, checksum);
}

void bench_float_arithmetic()
{
    double checksum = 0.0;
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 500000; ++i)
    {
        checksum += static_cast<double>(i) / 2.0;
    }
    report("float_arithmetic", started,
           static_cast<std::int64_t>(checksum));
}

void bench_overloads()
{
    std::int64_t checksum = runtime_seed();
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 100000; ++i)
    {
        checksum += add(i, std::int64_t{3}) +
            static_cast<std::int64_t>(add(1.0, 2.0));
        checksum -= checksum / 1000000;
    }
    report("overloads", started, checksum);
}

void bench_named_arguments()
{
    std::int64_t checksum = runtime_seed();
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 200000; ++i)
    {
        // C++ 没有命名实参，编译期绑定后调用相同的参数顺序。
        checksum += add(i, std::int64_t{3});
        checksum -= checksum / 1000000;
    }
    report("named_arguments", started, checksum);
}

void bench_recursion()
{
    const std::int64_t offset = runtime_seed();
    std::int64_t checksum = 0;
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 10000; ++i)
    {
        const std::int64_t depth = 20 + offset + i / 1000 -
            (i / 10000) * 10;
        checksum += recursive_sum(depth);
    }
    report("recursion", started, checksum);
}

void bench_variadic_unpack()
{
    dynamic_array extra{std::int64_t{2}, std::int64_t{3}};
    named_values named{{"bonus", std::int64_t{4}}};
    std::int64_t checksum = 0;
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 10000; ++i)
    {
        checksum += collect(i, extra, named);
    }
    report("variadic_unpack", started, checksum);
}

void bench_string_conversion()
{
    std::int64_t checksum = 0;
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 50000; ++i)
    {
        const std::string encoded = std::to_string(i);
        checksum += std::stoll(encoded);
        checksum += static_cast<std::int64_t>(("id:" + encoded).size());
    }
    report("string_conversion", started, checksum);
}
