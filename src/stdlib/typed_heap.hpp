#pragma once

#include "stdlib/ordered_compare.hpp"
#include "stdlib/priority_entry.hpp"
#include "stdlib/typed_container.hpp"
#include "stdlib/vector.hpp"

#include <algorithm>
#include <array>
#include <any>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <vector>

namespace tx_generated
{

template<class element_type>
struct heap_storage;

template<class element_type>
void register_heap_storage(const std::shared_ptr<heap_storage<element_type>>& storage);

template<class element_type>
struct heap_storage final : container_model<heap_storage<element_type>>,
                            graph_copyable
{
    struct node
    {
        element_type value;
        std::uint64_t sequence = 0;
    };

    std::vector<node> values;
    bool descending = false;
    std::string element_name;
    ordered_compare<element_type> order;
    std::uint64_t next_sequence = 0;

    explicit heap_storage(bool descending = false,
                          std::string element_name = {},
                          const void* less = nullptr,
                          std::any closure = {})
        : descending(descending), element_name(std::move(element_name)),
          order(less, std::move(closure))
    {
    }

    [[nodiscard]] std::string type_name() const override
    {
        const auto name = std::is_same_v<element_type, std::any>
            ? element_name : scalar_name<element_type>();
        return "heap<" + name + ">";
    }

    void require_ready() const
    {
        if (*order.invoking)
        {
            throw std::runtime_error("heap 比较器不能递归访问同一堆");
        }
    }

    [[nodiscard]] bool outranks(const node& left, const node& right) const
    {
        bool left_less = false;
        bool right_less = false;
        if constexpr (std::is_same_v<element_type, std::any>)
        {
            if (element_name.starts_with("priority_entry<"))
            {
                const auto left_priority = priority_of(left.value);
                const auto right_priority = priority_of(right.value);
                left_less = left_priority < right_priority;
                right_less = right_priority < left_priority;
            }
            else
            {
                left_less = order(left.value, right.value);
                right_less = order(right.value, left.value);
            }
        }
        else
        {
            if (!order.closure.has_value())
            {
                // 元素进入堆时已检查 NaN；默认顺序没有用户回调或再次校验。
                left_less = scalar_value(left.value) < scalar_value(right.value);
                right_less = scalar_value(right.value) < scalar_value(left.value);
            }
            else
            {
                left_less = order(left.value, right.value);
                right_less = order(right.value, left.value);
            }
        }
        if (left_less || right_less)
        {
            return descending ? right_less : left_less;
        }
        return left.sequence < right.sequence;
    }

    void push(const element_type& value)
    {
        require_ready();
        require_ordered_key(value);
        if (next_sequence == std::numeric_limits<std::uint64_t>::max())
        {
            throw std::overflow_error("heap 稳定序号已耗尽");
        }
        node pending{value, next_sequence};
        // 二叉堆路径最多 size_t 位数层；比较全部成功后才改动已有元素。
        std::array<std::size_t, std::numeric_limits<std::size_t>::digits> parents;
        std::size_t parent_count = 0;
        auto slot = values.size();
        while (slot != 0)
        {
            const auto parent = (slot - 1) / 2;
            if (!outranks(pending, values[parent]))
            {
                break;
            }
            parents[parent_count++] = parent;
            slot = parent;
        }
        values.push_back(pending);
        auto current = values.size() - 1;
        for (std::size_t index = 0; index < parent_count; ++index)
        {
            const auto parent = parents[index];
            values[current] = std::move(values[parent]);
            current = parent;
        }
        values[current] = std::move(pending);
        ++next_sequence;
    }

    [[nodiscard]] element_type top() const
    {
        require_ready();
        if (values.empty())
        {
            throw std::out_of_range("空 heap 不能 top");
        }
        return values.front().value;
    }

    void pop()
    {
        require_ready();
        if (values.empty())
        {
            throw std::out_of_range("空 heap 不能 pop");
        }
        if (values.size() == 1)
        {
            values.pop_back();
            return;
        }
        node pending = values.back();
        std::array<std::size_t, std::numeric_limits<std::size_t>::digits> children;
        std::size_t child_count = 0;
        std::size_t slot = 0;
        const auto limit = values.size() - 1;
        while (slot < limit / 2)
        {
            auto child = slot * 2 + 1;
            if (child + 1 < limit && outranks(values[child + 1], values[child]))
            {
                ++child;
            }
            if (!outranks(values[child], pending))
            {
                break;
            }
            children[child_count++] = child;
            slot = child;
        }
        slot = 0;
        for (std::size_t index = 0; index < child_count; ++index)
        {
            const auto child = children[index];
            values[slot] = std::move(values[child]);
            slot = child;
        }
        values[slot] = std::move(pending);
        values.pop_back();
    }

    void assign(const tx_vector<element_type>& input)
    {
        require_ready();
        std::vector<node> rebuilt;
        rebuilt.reserve(input.data().values.size());
        std::uint64_t sequence = 0;
        for (const auto& value : input.data().values)
        {
            require_ordered_key(value);
            if (sequence == std::numeric_limits<std::uint64_t>::max())
            {
                throw std::overflow_error("heap 稳定序号已耗尽");
            }
            rebuilt.push_back({value, sequence++});
        }
        for (auto index = rebuilt.size() / 2; index > 0; --index)
        {
            sift_down(rebuilt, index - 1);
        }
        values.swap(rebuilt);
        next_sequence = sequence;
    }

    [[nodiscard]] tx_vector<element_type> snapshot() const
    {
        require_ready();
        tx_vector<element_type> result(element_name);
        result.data().values.reserve(values.size());
        auto remaining = *this;
        while (!remaining.values.empty())
        {
            result.data().values.push_back(remaining.top());
            remaining.pop();
        }
        result.data().refresh();
        return result;
    }

    [[nodiscard]] container_handle empty_graph_copy() const override
    {
        auto result = std::make_shared<heap_storage>(descending,
            element_name, order.less, std::any{});
        register_heap_storage(result);
        return result;
    }

    void fill_graph_copy(container_storage& target,
                         const graph_copy_function& copy) const override
    {
        auto& destination = static_cast<heap_storage&>(target);
        destination.order = ordered_compare<element_type>(
            order.less, copy(order.closure));
        destination.next_sequence = next_sequence;
        destination.values.reserve(values.size());
        for (const auto& item : values)
        {
            if constexpr (std::is_same_v<element_type, std::any>)
            {
                destination.values.push_back({copy(item.value), item.sequence});
            }
            else
            {
                destination.values.push_back(item);
            }
        }
    }

    [[nodiscard]] std::string repr() const override
    {
        return type_name() + (descending ? "(max)" : "(min)") +
               format_repr_value(snapshot());
    }

private:
    void sift_down(std::vector<node>& items, std::size_t slot) const
    {
        const node pending = items[slot];
        while (slot * 2 + 1 < items.size())
        {
            auto child = slot * 2 + 1;
            if (child + 1 < items.size() && outranks(items[child + 1], items[child]))
            {
                ++child;
            }
            if (!outranks(items[child], pending))
            {
                break;
            }
            items[slot] = std::move(items[child]);
            slot = child;
        }
        items[slot] = pending;
    }
};

template<class element_type>
void register_heap_storage(const std::shared_ptr<heap_storage<element_type>>& storage)
{
    register_gc_node(storage,
        [](const void* object, gc_visit visit, void* context)
        {
            const auto& data = *static_cast<const heap_storage<element_type>*>(object);
            visit(data.order.closure, context);
            if constexpr (std::is_same_v<element_type, std::any>)
            {
                for (const auto& item : data.values)
                {
                    visit(item.value, context);
                }
            }
        },
        [](void* object)
        {
            auto& data = *static_cast<heap_storage<element_type>*>(object);
            data.values.clear();
            data.order.closure.reset();
        });
}

} // namespace tx_generated
