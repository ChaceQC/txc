#include <chrono>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

void scan(std::int64_t size, std::int64_t repeats, const char* name)
{
    std::vector<std::int64_t> values;
    for (std::int64_t index = 1; index <= size; ++index)
    {
        values.push_back(index);
    }
    std::int64_t checksum = 0;
    const auto started = std::chrono::steady_clock::now();
    for (std::int64_t round = 0; round < repeats; ++round)
    {
        for (const auto value : values)
        {
            if (checksum > std::numeric_limits<std::int64_t>::max() - value)
            {
                throw std::overflow_error("整数加法溢出");
            }
            checksum += value;
        }
    }
    const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - started).count();
    std::cout << name << '\n' << micros << '\n' << checksum << '\n';
}

int main()
{
    scan(1000, 1000, "vector_1k");
    scan(100000, 10, "vector_100k");
}
