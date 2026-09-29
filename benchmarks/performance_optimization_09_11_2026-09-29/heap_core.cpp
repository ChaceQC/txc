#include "stdlib/typed_heap.hpp"

#include <chrono>
#include <cstdio>
#include <cstdint>

int main()
{
    tx_generated::heap_storage<std::int64_t> values;
    std::int64_t checksum = 0;
    const auto start = std::chrono::steady_clock::now();
    for (std::int64_t index = 0; index < 50000; ++index)
    {
        values.push(index % 1000);
    }
    for (std::int64_t index = 0; index < 50000; ++index)
    {
        checksum += values.top();
        values.pop();
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - start).count();
    std::printf("heap_push_pop\n%lld\n%lld\n", static_cast<long long>(elapsed),
                static_cast<long long>(checksum));
    return checksum == 24975000 ? 0 : 1;
}
