#include "stdlib/profile.hpp"

#include <cstdlib>
#include <malloc.h>
#include <new>

namespace
{

void* allocate(std::size_t size, std::size_t alignment)
{
    while (true)
    {
#ifdef _WIN32
        void* result = alignment ? _aligned_malloc(size ? size : 1, alignment)
                                 : std::malloc(size ? size : 1);
#else
        void* result = nullptr;
        if (alignment)
        {
            if (posix_memalign(&result, alignment, size ? size : 1) != 0)
            {
                result = nullptr;
            }
        }
        else
        {
            result = std::malloc(size ? size : 1);
        }
#endif
        if (result)
        {
            tx_generated::profile_allocated(result, size);
            return result;
        }
        const auto handler = std::get_new_handler();
        if (!handler)
        {
            throw std::bad_alloc();
        }
        handler();
    }
}

void deallocate(void* pointer, bool aligned) noexcept
{
    if (pointer)
    {
        tx_generated::profile_freed(pointer);
#ifdef _WIN32
        if (aligned)
        {
            _aligned_free(pointer);
        }
        else
        {
            std::free(pointer);
        }
#else
        (void)aligned;
        std::free(pointer);
#endif
    }
}

} // namespace

namespace tx_generated
{
void profile_link_allocator() noexcept
{
}
} // namespace tx_generated

void* operator new(std::size_t size)
{
    return allocate(size, 0);
}

void* operator new[](std::size_t size)
{
    return allocate(size, 0);
}

void* operator new(std::size_t size, std::align_val_t alignment)
{
    return allocate(size, static_cast<std::size_t>(alignment));
}

void* operator new[](std::size_t size, std::align_val_t alignment)
{
    return allocate(size, static_cast<std::size_t>(alignment));
}

void operator delete(void* pointer) noexcept
{
    deallocate(pointer, false);
}

void operator delete[](void* pointer) noexcept
{
    deallocate(pointer, false);
}

void operator delete(void* pointer, std::size_t) noexcept
{
    deallocate(pointer, false);
}

void operator delete[](void* pointer, std::size_t) noexcept
{
    deallocate(pointer, false);
}

void operator delete(void* pointer, std::align_val_t) noexcept
{
    deallocate(pointer, true);
}

void operator delete[](void* pointer, std::align_val_t) noexcept
{
    deallocate(pointer, true);
}

void operator delete(void* pointer, std::size_t, std::align_val_t) noexcept
{
    deallocate(pointer, true);
}

void operator delete[](void* pointer, std::size_t, std::align_val_t) noexcept
{
    deallocate(pointer, true);
}
