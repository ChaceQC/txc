#pragma once

#include <any>
#include <cstdint>
#include <string>
#include <variant>

namespace tx_generated
{

// 结果状态保存值而不是工作线程创建的 C ABI 句柄。
using concurrent_result = std::variant<std::monostate, std::int64_t,
    double, bool, std::string, std::any>;

concurrent_result invoke_concurrent_callback(const void* target,
    const void* callback, std::int64_t kind);

} // namespace tx_generated
