#pragma once

#include "backend/cpp/cycle_gc.hpp"

#include <cstddef>
#include <memory>
#include <string>

namespace tx_generated
{

// 虚接口只服务 print、any 和 deep_copy；静态操作直接访问具体存储类型。
struct container_storage
{
    virtual ~container_storage() = default;
    [[nodiscard]] virtual std::string type_name() const = 0;
    [[nodiscard]] virtual std::string repr() const = 0;
    [[nodiscard]] virtual std::size_t size() const = 0;
    [[nodiscard]] virtual std::shared_ptr<container_storage> copy() const = 0;
};

using container_handle = std::shared_ptr<container_storage>;

template<class storage_type>
struct container_model : container_storage
{
    [[nodiscard]] std::size_t size() const override
    {
        return static_cast<const storage_type&>(*this).values.size();
    }

    [[nodiscard]] container_handle copy() const override
    {
        auto result = std::make_shared<storage_type>(static_cast<const storage_type&>(*this));
        note_gc_allocation();
        return result;
    }
};

} // namespace tx_generated
