#include "backend/cpp/graphics_abi.hpp"

extern "C" void* txrt_graphics_resource_view(const void* value) noexcept
{
    // 在值绑定/投影视图时解包；绘图命令接收缓存的控制块指针。
    const auto* item = std::any_cast<tx_generated::graphics::handle>(
        static_cast<const std::any*>(value));
    return item && item->owner ? item->owner->target.get() : nullptr;
}
