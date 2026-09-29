#pragma once

#include "stdlib/decimal.hpp"

namespace tx_generated::decimal_math
{

// 数据库桥接复用 decimal 原有的类型检查和对象构造，不重新解释布局。
value read(const void* source);
void* make(value number, const char* type_name);

} // namespace tx_generated::decimal_math
