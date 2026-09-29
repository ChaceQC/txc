#include "stdlib/typed_heap.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <new>

namespace
{
bool counting = false;
std::size_t allocations = 0;
}

void* operator new(std::size_t size)
{
    if (counting)
    {
        ++allocations;
    }
    if (auto* result = std::malloc(size ? size : 1))
    {
        return result;
    }
    throw std::bad_alloc();
}

void operator delete(void* value) noexcept
{
    std::free(value);
}

void operator delete(void* value, std::size_t) noexcept
{
    std::free(value);
}

int main()
{
    tx_generated::heap_storage<std::int64_t> heap;
    heap.values.reserve(4096);
    counting = true;
    for (std::int64_t index = 4095; index >= 0; --index)
    {
        heap.push(index);
    }
    for (std::int64_t index = 0; index < 4096; ++index)
    {
        assert(heap.top() == index);
        heap.pop();
    }
    counting = false;
    assert(allocations == 0);
    heap.next_sequence = std::numeric_limits<std::uint64_t>::max();
    try
    {
        heap.push(1);
        return 1;
    }
    catch (const std::overflow_error&)
    {
        assert(heap.values.empty());
    }
    for (bool descending : {false, true})
    {
        tx_generated::heap_storage<double> values(descending);
        values.push(-0.0);
        values.push(0.0);
        assert(std::signbit(values.top()));
        values.pop();
        assert(!std::signbit(values.top()));
        try
        {
            values.push(std::numeric_limits<double>::quiet_NaN());
            return 2;
        }
        catch (const std::runtime_error&)
        {
            assert(values.values.size() == 1);
        }
    }
    std::puts("heap: zero path allocations, stable zeros, sequence exhaustion, NaN");
}
