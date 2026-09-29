#include <chrono>
#include <cstdint>
#include <iostream>

extern "C" void txrt_stack_push(const char*, const char*, std::size_t,
    std::size_t) noexcept;
extern "C" void txrt_stack_location(const char*, std::size_t,
    std::size_t) noexcept;
extern "C" void txrt_stack_pop() noexcept;
extern "C" int txrt_gc_safepoint() noexcept;

using clock_type = std::chrono::steady_clock;

int main()
{
    constexpr int count = 1000000;
    txrt_stack_push("bench", "bench.tx", 1, 1);
    auto start = clock_type::now();
    for (int i = 0; i < count; ++i)
    {
        txrt_stack_location("bench.tx", 2, 1);
    }
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        clock_type::now() - start).count();
    txrt_stack_pop();
    std::cout << "stack_location_us " << elapsed << '\n';
    start = clock_type::now();
    std::int64_t checksum = 0;
    for (int i = 0; i < count; ++i)
    {
        checksum += txrt_gc_safepoint();
    }
    elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        clock_type::now() - start).count();
    std::cout << "gc_safepoint_us " << elapsed << '\n';
    start = clock_type::now();
    for (int i = 0; i < count; ++i)
    {
        txrt_stack_push("bench", "bench.tx", 1, 1);
        txrt_stack_pop();
    }
    elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        clock_type::now() - start).count();
    std::cout << "stack_push_pop_us " << elapsed << '\n';
    std::cout << "checksum " << checksum << '\n';
}
