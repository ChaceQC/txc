#pragma once

#include <any>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace tx_generated
{

// LLVM 与运行时共享：每槽 8 字节、8 字节对齐，引用槽持有独立 any 值而非 GC 根。
enum class slot_kind : std::uint8_t
{
    reference = 0,
    integer = 1,
    floating = 2,
    boolean = 3
};

union alignas(8) typed_slot
{
    std::int64_t integer = 0;
    double floating;
    bool boolean;
    std::any* reference;
};

static_assert(sizeof(typed_slot) == 8 && alignof(typed_slot) == 8);

class typed_slots
{
public:
    typed_slots() = default;
    explicit typed_slots(std::span<const slot_kind> kinds, bool borrow_kinds = false);
    typed_slots(typed_slots&& other) noexcept;
    typed_slots& operator=(typed_slots&& other) noexcept;
    typed_slots(const typed_slots&) = delete;
    typed_slots& operator=(const typed_slots&) = delete;

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] typed_slot* data() noexcept;
    [[nodiscard]] const typed_slot* data() const noexcept;
    [[nodiscard]] slot_kind kind(std::size_t index) const;
    [[nodiscard]] std::any read(std::size_t index) const;
    [[nodiscard]] std::any& reference(std::size_t index) const;
    void write(std::size_t index, std::any value);
    void clear() noexcept;

private:
    void refresh_references() noexcept;
    std::vector<slot_kind> owned_kinds_;
    std::span<const slot_kind> kinds_;
    std::array<typed_slot, 2> local_{};
    std::vector<typed_slot> slots_;
    std::array<std::any, 2> local_references_;
    std::vector<std::any> references_;
};

} // namespace tx_generated
