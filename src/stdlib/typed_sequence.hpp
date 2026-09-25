#pragma once

#include "stdlib/container_scalar.hpp"
#include "stdlib/typed_container.hpp"
#include "stdlib/vector.hpp"

#include <algorithm>
#include <deque>

namespace tx_generated
{

template<class element_type>
struct heap_storage final : container_model<heap_storage<element_type>>
{
    std::vector<element_type> values;
    heap_compare<element_type> compare;

    explicit heap_storage(bool descending = false) : compare{descending}
    {
    }

    [[nodiscard]] std::string type_name() const override
    {
        return "heap<" + scalar_name<element_type>() + ">";
    }

    void push(const element_type& value)
    {
        require_ordered_key(value);
        values.push_back(value);
        std::push_heap(values.begin(), values.end(), compare);
    }

    [[nodiscard]] element_type top() const
    {
        if (values.empty())
        {
            throw std::out_of_range("空 heap 不能 top");
        }
        return values.front();
    }

    void pop()
    {
        if (values.empty())
        {
            throw std::out_of_range("空 heap 不能 pop");
        }
        std::pop_heap(values.begin(), values.end(), compare);
        values.pop_back();
    }

    [[nodiscard]] tx_vector<element_type> snapshot() const
    {
        tx_vector<element_type> result;
        result.data().values = values;
        // sort_heap 产生的是比较器升序；逆转后才是逐次出堆的顺序。
        auto& items = result.data().values;
        std::sort_heap(items.begin(), items.end(), compare);
        std::reverse(items.begin(), items.end());
        result.data().refresh();
        return result;
    }

    [[nodiscard]] std::string repr() const override
    {
        return type_name() + (compare.descending ? "(max)" : "(min)") +
               format_repr_value(snapshot());
    }
};

template<class element_type>
struct queue_storage final : container_model<queue_storage<element_type>>
{
    std::deque<element_type> values;

    [[nodiscard]] std::string type_name() const override
    {
        return "queue<" + scalar_name<element_type>() + ">";
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

    [[nodiscard]] tx_vector<element_type> snapshot() const
    {
        tx_vector<element_type> result;
        result.data().values.assign(values.begin(), values.end());
        result.data().refresh();
        return result;
    }

    [[nodiscard]] std::string repr() const override
    {
        return type_name() + format_repr_value(snapshot());
    }
};

} // namespace tx_generated
