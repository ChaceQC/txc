#pragma once

#include "stdlib/container_scalar.hpp"
#include "stdlib/typed_container.hpp"
#include "stdlib/vector.hpp"

#include <any>
#include <cstdint>
#include <deque>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_set>

namespace tx_generated
{

template<class element_type>
struct deque_storage final : container_model<deque_storage<element_type>>
{
    std::deque<element_type> values;
    std::string element_name;

    explicit deque_storage(std::string name = {}) : element_name(std::move(name))
    {
    }

    [[nodiscard]] std::string type_name() const override
    {
        if constexpr (std::is_same_v<element_type, std::any>)
        {
            return "deque<" + element_name + ">";
        }
        else
        {
            return "deque<" + scalar_name<element_type>() + ">";
        }
    }

    [[nodiscard]] static std::size_t checked_index(std::int64_t index)
    {
        if (index < 0)
        {
            throw std::out_of_range("deque 索引不能为负数");
        }
        return static_cast<std::size_t>(index);
    }

    [[nodiscard]] element_type read(std::int64_t index) const
    {
        const auto at = checked_index(index);
        if (at >= values.size())
        {
            throw std::out_of_range("deque 索引越界");
        }
        return values[at];
    }

    void set(std::int64_t index, const element_type& value)
    {
        const auto at = checked_index(index);
        if (at >= values.size())
        {
            throw std::out_of_range("deque 索引越界");
        }
        values[at] = value;
    }

    void insert(std::int64_t index, const element_type& value)
    {
        const auto at = checked_index(index);
        if (at > values.size())
        {
            throw std::out_of_range("deque 插入位置越界");
        }
        values.insert(values.begin() + at, value);
    }

    void erase(std::int64_t index)
    {
        const auto at = checked_index(index);
        if (at >= values.size())
        {
            throw std::out_of_range("deque 删除位置越界");
        }
        values.erase(values.begin() + at);
    }

    [[nodiscard]] element_type front() const
    {
        if (values.empty())
        {
            throw std::out_of_range("空 deque 不能 front");
        }
        return values.front();
    }

    [[nodiscard]] element_type back() const
    {
        if (values.empty())
        {
            throw std::out_of_range("空 deque 不能 back");
        }
        return values.back();
    }

    void pop_front()
    {
        if (values.empty())
        {
            throw std::out_of_range("空 deque 不能 pop_front");
        }
        values.pop_front();
    }

    void pop_back()
    {
        if (values.empty())
        {
            throw std::out_of_range("空 deque 不能 pop_back");
        }
        values.pop_back();
    }

    [[nodiscard]] tx_vector<element_type> snapshot() const
    {
        tx_vector<element_type> result(element_name);
        result.data().values.assign(values.begin(), values.end());
        result.data().refresh();
        return result;
    }

    [[nodiscard]] std::string repr() const override
    {
        // 复合元素可以通过 class 字段回指本队列，打印时截断重复节点。
        static thread_local std::unordered_set<const void*> active;
        if (!active.insert(this).second)
        {
            return type_name() + "[...]";
        }
        struct active_guard
        {
            std::unordered_set<const void*>& nodes;
            const void* value;
            ~active_guard()
            {
                nodes.erase(value);
            }
        } guard{active, this};
        std::string result = type_name() + "[";
        bool first = true;
        for (const auto& value : values)
        {
            result += first ? "" : ", ";
            first = false;
            if constexpr (std::is_same_v<element_type, std::any>)
            {
                result += format_repr_value(value);
            }
            else
            {
                result += format_repr_value(scalar_value(value));
            }
        }
        return result + "]";
    }
};

using object_deque_storage = deque_storage<std::any>;

inline void register_object_deque(const std::shared_ptr<object_deque_storage>& value)
{
    register_gc_node(value,
        [](const void* object, gc_visit visit, void* context)
        {
            for (const auto& item : static_cast<const object_deque_storage*>(object)->values)
            {
                visit(item, context);
            }
        },
        [](void* object)
        {
            static_cast<object_deque_storage*>(object)->values.clear();
        });
}

} // namespace tx_generated
