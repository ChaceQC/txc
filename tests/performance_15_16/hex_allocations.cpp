#include "stdlib/bytes.hpp"
#include "stdlib/error.hpp"

#include <cstdlib>
#include <iostream>
#include <new>
#include <string>

namespace
{

std::size_t allocation_count = 0;
std::size_t large_allocation_count = 0;
bool counting = false;

} // namespace

void* operator new(std::size_t size)
{
    if (counting)
    {
        ++allocation_count;
        large_allocation_count += size >= 65536;
    }
    if (void* result = std::malloc(size == 0 ? 1 : size))
    {
        return result;
    }
    throw std::bad_alloc();
}

void operator delete(void* pointer) noexcept
{
    std::free(pointer);
}

void operator delete(void* pointer, std::size_t) noexcept
{
    std::free(pointer);
}

int main()
{
    auto data = tx_generated::make_bytes(std::vector<std::uint8_t>(65536, 255));
    counting = true;
    auto encoded = tx_generated::bytes_to_hex(data);
    counting = false;
    if (allocation_count != 1 || encoded != std::string(131072, 'f'))
    {
        return 1;
    }
    allocation_count = 0;
    counting = true;
    auto decoded = tx_generated::bytes_from_hex(encoded);
    counting = false;
    // 一个连续字节缓冲，另一次分配是不可变拥有者/控制块。
    if (allocation_count != 2 || *decoded != *data)
    {
        return 2;
    }
    for (const auto position : {std::size_t{0}, encoded.size() - 1})
    {
        for (const auto invalid : {0, 128, 255, static_cast<int>('g')})
        {
            std::string bad = encoded;
            bad[position] = static_cast<char>(invalid);
            large_allocation_count = 0;
            bool rejected = false;
            counting = true;
            try
            {
                (void)tx_generated::bytes_from_hex(bad);
            }
            catch (const tx_generated::runtime_failure&)
            {
                rejected = true;
            }
            counting = false;
            if (!rejected || large_allocation_count != 0)
            {
                return 3;
            }
        }
    }
    std::cout << "PASS one output buffer; invalid high bytes/NUL reject before output allocation\n";
}
