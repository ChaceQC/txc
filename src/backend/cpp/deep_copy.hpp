#pragma once

#include <any>

namespace tx_generated
{

std::any deep_copy_value(const std::any& value);
// 图复制分派之后的外部资源契约，拒绝不可复制句柄。
std::any copy_resource_value(const std::any& value);

} // namespace tx_generated
