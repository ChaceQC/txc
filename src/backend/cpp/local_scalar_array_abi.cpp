#include "backend/cpp/cycle_gc.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/value_abi.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>

namespace
{

// 与 LLVM 后端的 { i8, i64 } 槽位保持相同布局。
struct scalar_slot
{
    std::uint8_t kind;
    std::uint64_t bits;
};

static_assert(sizeof(scalar_slot) == 16);
static_assert(offsetof(scalar_slot, bits) == 8);

} // namespace

extern "C" int txrt_local_scalar_array_new(std::int64_t length,
                                             void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        if (length < 0)
        {
            throw std::runtime_error("数组长度不能为负");
        }
        auto slots = std::make_unique<scalar_slot[]>(
            static_cast<std::size_t>(length));
        for (std::int64_t index = 0; index < length; ++index)
        {
            slots[static_cast<std::size_t>(index)].kind = 1;
        }
        tx_generated::note_gc_allocation();
        *result = slots.release();
    });
}

extern "C" void txrt_local_scalar_array_release(void* value) noexcept
{
    delete[] static_cast<scalar_slot*>(value);
}
