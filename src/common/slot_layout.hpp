#pragma once

#include <cstddef>
#include <cstdint>

namespace tx
{

// 编译器与运行时共同维护槽位 ABI；不能独立修改一侧的编号或步长。
enum class slot_kind : std::uint8_t
{
    reference = 0,
    integer = 1,
    floating = 2,
    boolean = 3
};

inline constexpr std::size_t slot_bytes = 8;
inline constexpr std::size_t slot_alignment = 8;
// 限制单次原生特化的栈空间和展开量，不以语言字段数量定义语义。
inline constexpr std::size_t native_record_budget = 256;
inline constexpr std::size_t static_format_budget = 32;

} // namespace tx
