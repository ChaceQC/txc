#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <queue>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace
{

using clock_type = std::chrono::steady_clock;

struct heap_node
{
    std::int64_t value;
    std::uint64_t sequence;
};

struct later_first
{
    bool operator()(const heap_node& left, const heap_node& right) const
    {
        return left.value != right.value ? left.value > right.value
                                         : left.sequence > right.sequence;
    }
};

struct mean_accumulator
{
    std::int64_t count = 0;
    long double total = 0;
    long double correction = 0;

    static constexpr bool wide_sum = std::numeric_limits<long double>::max_exponent >
        std::numeric_limits<double>::max_exponent +
        std::numeric_limits<std::int64_t>::digits + 1;

    void add(double value)
    {
        if (!std::isfinite(value) || count == std::numeric_limits<std::int64_t>::max())
        {
            throw std::runtime_error("统计样本或计数无效");
        }
        ++count;
        const auto sample = static_cast<long double>(value);
        if constexpr (wide_sum)
        {
            const auto next = total + sample;
            correction += std::fabs(total) >= std::fabs(sample)
                ? (total - next) + sample : (sample - next) + total;
            total = next;
        }
        else
        {
            total = std::signbit(total) == std::signbit(sample)
                ? total + (sample - total) / count
                : total * (static_cast<long double>(count - 1) / count) + sample / count;
        }
    }

    double finish() const
    {
        if (count == 0)
        {
            throw std::runtime_error("空样本没有均值");
        }
        const auto result = wide_sum ? (total + correction) / count : total;
        if (!std::isfinite(result) ||
            std::fabs(result) > std::numeric_limits<double>::max())
        {
            throw std::runtime_error("统计结果超出有限 float 范围");
        }
        return static_cast<double>(result);
    }
};

template<class operation_type>
void report(std::string_view name, operation_type operation)
{
    const auto started = clock_type::now();
    const auto checksum = operation();
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        clock_type::now() - started).count();
    std::cout << name << '\n' << elapsed << '\n' << checksum << '\n';
}

std::int64_t heap_push_pop()
{
    std::priority_queue<heap_node, std::vector<heap_node>, later_first> values;
    for (std::uint64_t index = 0; index < 50000; ++index)
    {
        values.push({static_cast<std::int64_t>(index % 1000), index});
    }
    std::int64_t checksum = 0;
    for (int index = 0; index < 50000; ++index)
    {
        checksum += values.top().value;
        values.pop();
    }
    return checksum;
}

std::int64_t statistics_mean()
{
    std::vector<double> values;
    values.reserve(128);
    for (int index = 0; index < 128; ++index)
    {
        values.push_back(index);
    }
    std::int64_t checksum = 0;
    for (int repetition = 0; repetition < 10000; ++repetition)
    {
        mean_accumulator state;
        for (const auto value : values)
        {
            state.add(value);
        }
        checksum += static_cast<std::int64_t>(state.finish());
    }
    return checksum;
}

void check_contracts()
{
    std::priority_queue<heap_node, std::vector<heap_node>, later_first> values;
    values.push({7, 2});
    values.push({7, 0});
    values.push({7, 1});
    for (std::uint64_t expected = 0; expected < 3; ++expected)
    {
        if (values.top().sequence != expected)
        {
            throw std::runtime_error("堆同优先级稳定性校验失败");
        }
        values.pop();
    }
    mean_accumulator state;
    state.add(1e16);
    state.add(1);
    state.add(-1e16);
    if (state.finish() != 1.0 / 3.0)
    {
        throw std::runtime_error("补偿均值校验失败");
    }
}

}

int main(int argc, char** argv)
{
    if (argc == 2 && std::string_view{argv[1]} == "--contract-check")
    {
        check_contracts();
        return 0;
    }
    if (argc != 1)
    {
        throw std::runtime_error("未知的基准参数");
    }
    report("heap_push_pop", heap_push_pop);
    report("statistics_mean", statistics_mean);
}
