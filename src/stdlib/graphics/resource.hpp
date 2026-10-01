#pragma once

#include "common/graphics_types.hpp"

#include <any>
#include <atomic>
#include <memory>
#include <string_view>

namespace tx_generated::graphics
{

struct resource
{
    explicit resource(tx::graphics_kind value) : kind(value)
    {
    }
    virtual ~resource() = default;
    tx::graphics_kind kind;
    std::atomic<bool> abandoned{false};
};

struct lease
{
    explicit lease(std::shared_ptr<resource> value) : target(std::move(value))
    {
    }
    ~lease()
    {
        // 最终值可能由 GC 释放；只标记，原生销毁由 UI 线程执行。
        target->abandoned.store(true, std::memory_order_release);
    }
    std::shared_ptr<resource> target;
};

struct handle
{
    std::shared_ptr<lease> owner;
};

inline handle make_handle(std::shared_ptr<resource> value)
{
    value->abandoned.store(false, std::memory_order_release);
    return {std::make_shared<lease>(std::move(value))};
}

inline bool matches(const std::any& value, std::string_view name) noexcept
{
    const auto* item = std::any_cast<handle>(&value);
    return item && item->owner && item->owner->target->kind == tx::graphics_type_kind(name);
}

} // namespace tx_generated::graphics
