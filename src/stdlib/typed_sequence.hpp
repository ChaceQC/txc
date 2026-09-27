#pragma once

#include "stdlib/container_scalar.hpp"
#include "stdlib/typed_heap.hpp"
#include "stdlib/typed_container.hpp"
#include "stdlib/vector.hpp"

#include <algorithm>
#include <any>
#include <deque>
#include <memory>
#include <string>
#include <type_traits>

namespace tx_generated
{
template<class element_type>
struct queue_storage;

template<class element_type>
void register_queue_storage(
    const std::shared_ptr<queue_storage<element_type>>& storage);

template<class element_type>
struct queue_storage final : container_model<queue_storage<element_type>>,
                             graph_copyable
{
    static constexpr bool has_user_effects = std::is_same_v<element_type, std::any>;

    std::deque<element_type> values;
    std::string element_name;

    explicit queue_storage(std::string name = {}) : element_name(std::move(name))
    {
    }

    [[nodiscard]] std::string type_name() const override
    {
        const auto name = std::is_same_v<element_type, std::any>
            ? element_name : scalar_name<element_type>();
        return "queue<" + name + ">";
    }

    void push(const element_type& value)
    {
        values.push_back(value);
    }

    [[nodiscard]] element_type front() const
    {
        if (values.empty())
        {
            throw std::out_of_range("空 queue 不能 front");
        }
        return values.front();
    }

    [[nodiscard]] element_type back() const
    {
        if (values.empty())
        {
            throw std::out_of_range("空 queue 不能 back");
        }
        return values.back();
    }

    void pop()
    {
        if (values.empty())
        {
            throw std::out_of_range("空 queue 不能 pop");
        }
        values.pop_front();
    }

    void assign(const tx_vector<element_type>& input)
    {
        std::deque<element_type> rebuilt(
            input.data().values.begin(), input.data().values.end());
        values.swap(rebuilt);
    }

    [[nodiscard]] tx_vector<element_type> snapshot() const
    {
        tx_vector<element_type> result(element_name);
        result.data().values.assign(values.begin(), values.end());
        result.data().refresh();
        return result;
    }

    [[nodiscard]] container_handle empty_graph_copy() const override
    {
        auto result = std::make_shared<queue_storage>(element_name);
        register_queue_storage(result);
        return result;
    }

    void fill_graph_copy(container_storage& target,
                         const graph_copy_function& copy) const override
    {
        auto& destination = static_cast<queue_storage&>(target).values;
        for (const auto& value : values)
        {
            if constexpr (std::is_same_v<element_type, std::any>)
            {
                destination.push_back(copy(value));
            }
            else
            {
                destination.push_back(value);
            }
        }
    }

    [[nodiscard]] std::string repr() const override
    {
        return type_name() + format_repr_value(snapshot());
    }
};

template<class element_type>
void register_queue_storage(
    const std::shared_ptr<queue_storage<element_type>>& storage)
{
    if constexpr (std::is_same_v<element_type, std::any>)
    {
        register_gc_node(storage,
            [](const void* object, gc_visit visit, void* context)
            {
                for (const auto& value :
                     static_cast<const queue_storage<element_type>*>(object)->values)
                {
                    visit(value, context);
                }
            },
            [](void* object)
            {
                static_cast<queue_storage<element_type>*>(object)->values.clear();
            });
    }
    else
    {
        note_gc_allocation();
    }
}

} // namespace tx_generated
