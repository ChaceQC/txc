#pragma once

#include <cstdint>

namespace tx
{

// 静态字段的复制分派；标量由槽 kind 直接复制，unknown 保留资源错误契约。
enum class record_copy_kind : std::uint64_t
{
    unknown,
    value,
    structure,
    class_object,
    array,
    dictionary,
    vector_i64,
    vector_f64,
    vector_bool,
    vector_str,
    vector_bytes,
    vector_object,
    container,
    iterator,
    closure
};

} // namespace tx
