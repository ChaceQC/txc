#include <algorithm>
#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <iostream>
#include <optional>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using clock_type = std::chrono::steady_clock;

template<class Operation>
void measure(const char* name, Operation operation)
{
    const auto start = clock_type::now();
    const auto checksum = operation();
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        clock_type::now() - start).count();
    std::cout << name << '\n' << elapsed << '\n' << checksum << '\n';
}

std::int64_t vector_push()
{
    std::vector<std::int64_t> values;
    for (int i = 0; i < 100000; ++i)
    {
        values.push_back(i);
    }
    return values[99999] + values.size();
}

std::int64_t vector_index()
{
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto seed = now.count() > 0 ? 7 : 8;
    std::vector<std::int64_t> values(1024, seed);
    std::int64_t checksum = 0;
    for (int i = 0; i < 500000; ++i)
    {
        checksum += values[i % 1024];
    }
    return checksum;
}

std::int64_t map_lookup()
{
    std::unordered_map<int, int> values;
    for (int i = 0; i < 128; ++i)
    {
        values[i] = i;
    }
    std::int64_t checksum = 0;
    for (int i = 0; i < 100000; ++i)
    {
        checksum += values.at(i % 128);
    }
    return checksum;
}

std::int64_t set_contains()
{
    std::unordered_set<int> values;
    for (int i = 0; i < 128; ++i)
    {
        values.insert(i);
    }
    std::int64_t checksum = 0;
    for (int i = 0; i < 100000; ++i)
    {
        checksum += values.contains(i % 128) ? 1 : 0;
    }
    return checksum;
}

std::int64_t heap_push_pop()
{
    std::priority_queue<int, std::vector<int>, std::greater<int>> values;
    for (int i = 0; i < 50000; ++i)
    {
        values.push(i % 1000);
    }
    std::int64_t checksum = 0;
    for (int i = 0; i < 50000; ++i)
    {
        checksum += values.top();
        values.pop();
    }
    return checksum;
}

std::int64_t queue_push_pop()
{
    std::deque<int> values;
    for (int i = 0; i < 100000; ++i)
    {
        values.push_back(i);
    }
    std::int64_t checksum = 0;
    for (int i = 0; i < 100000; ++i)
    {
        checksum += values.front();
        values.pop_front();
    }
    return checksum;
}

std::int64_t iterator_snapshot()
{
    std::vector<int> values;
    for (int i = 0; i < 1000; ++i)
    {
        values.push_back(i);
    }
    std::int64_t checksum = 0;
    for (int cycle = 0; cycle < 100; ++cycle)
    {
        auto snapshot = values;
        auto cursor = snapshot.begin();
        for (int i = 0; i < 1000; ++i)
        {
            checksum += *cursor++;
        }
    }
    return checksum;
}

std::int64_t option_value()
{
    std::int64_t checksum = 0;
    for (int i = 0; i < 100000; ++i)
    {
        std::optional<int> value{i % 8};
        checksum += value.value();
    }
    return checksum;
}

int double_value(int value)
{
    return value * 2;
}

std::int64_t function_value()
{
    std::function<int(int)> operation = double_value;
    std::int64_t checksum = 0;
    for (int i = 0; i < 100000; ++i)
    {
        checksum += operation(i);
    }
    return checksum;
}

int add(int base, int value)
{
    return base + value;
}

std::int64_t closure_bind()
{
    std::function<int(int)> operation = std::bind_front(add, 5);
    std::int64_t checksum = 0;
    for (int i = 0; i < 100000; ++i)
    {
        checksum += operation(i);
    }
    return checksum;
}

int main()
{
    measure("vector_push", vector_push);
    measure("vector_index", vector_index);
    measure("map_lookup", map_lookup);
    measure("set_contains", set_contains);
    measure("heap_push_pop", heap_push_pop);
    measure("queue_push_pop", queue_push_pop);
    measure("iterator_snapshot", iterator_snapshot);
    measure("option_value", option_value);
    measure("function_value", function_value);
    measure("closure_bind", closure_bind);
}
