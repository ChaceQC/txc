#include "stdlib/format_binding_cache.hpp"

namespace tx_generated
{

// 非平凡 TLS 对象仅在一个编译单元中定义，避免 MinGW 重复生成初始化符号。
thread_local std::list<std::shared_ptr<const format_binding_plan>> format_bindings;

} // namespace tx_generated
