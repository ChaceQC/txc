#pragma once

#include "backend/cpp/cycle_gc.hpp"

#include <cstddef>
#include <any>
#include <functional>
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

// 需要复制内部对象图的容器先建立空副本，再在复制上下文中填充。
// 这样值或比较器闭包回指容器时仍可保持环和共享关系。
using graph_copy_function = std::function<std::any(const std::any&)>;

struct graph_copyable
{
    virtual ~graph_copyable() = default;
    [[nodiscard]] virtual container_handle empty_graph_copy() const = 0;
    virtual void fill_graph_copy(container_storage& target,
                                 const graph_copy_function& copy) const = 0;
};

template<class storage_type>
struct container_model : container_storage
{
    static constexpr bool has_user_effects = true;

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
