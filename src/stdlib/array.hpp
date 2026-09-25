#pragma once

#include "backend/cpp/cycle_gc.hpp"

#include <algorithm>
#include <any>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <typeinfo>
#include <type_traits>
#include <utility>
#include <vector>

namespace tx_generated
{

class tx_array
{
public:
    using iterator = std::vector<std::any>::iterator;
    using const_iterator = std::vector<std::any>::const_iterator;

    tx_array() : elements_(std::make_shared<storage>())
    {
        note_gc_allocation();
    }
    explicit tx_array(std::size_t count)
        : elements_(std::make_shared<storage>(count))
    {
        note_gc_allocation();
    }

    template<class input_iterator>
    tx_array(input_iterator first, input_iterator last)
        : elements_(std::make_shared<storage>(first, last))
    {
        note_gc_allocation();
        if (std::any_of(elements_->values.begin(), elements_->values.end(),
                        may_contain_cycle))
        {
            register_storage();
        }
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return elements_->values.size();
    }
    [[nodiscard]] std::any& operator[](std::size_t index)
    {
        register_storage();
        return elements_->values[index];
    }
    [[nodiscard]] const std::any& operator[](std::size_t index) const
    {
        return elements_->values[index];
    }
    [[nodiscard]] iterator begin()
    {
        register_storage();
        return elements_->values.begin();
    }
    [[nodiscard]] iterator end()
    {
        register_storage();
        return elements_->values.end();
    }
    [[nodiscard]] const_iterator begin() const noexcept
    {
        return elements_->values.begin();
    }
    [[nodiscard]] const_iterator end() const noexcept
    {
        return elements_->values.end();
    }
    void reserve(std::size_t count)
    {
        elements_->values.reserve(count);
    }
    void push_back(std::any value)
    {
        if (may_contain_cycle(value))
        {
            register_storage();
        }
        elements_->values.push_back(std::move(value));
    }
    template<class... arguments>
    void emplace_back(arguments&&... values)
    {
        std::any value(std::forward<arguments>(values)...);
        push_back(std::move(value));
    }
    void insert(iterator where, const_iterator first, const_iterator last)
    {
        // 任意范围可能包含复合值，并且会使现有元素地址失效。
        register_storage();
        elements_->values.insert(where, first, last);
    }
    template<class scalar_type>
        requires (std::is_same_v<scalar_type, std::int64_t> ||
                  std::is_same_v<scalar_type, double> ||
                  std::is_same_v<scalar_type, bool>)
    void set_scalar(std::size_t index, scalar_type value)
    {
        elements_->values[index] = value;
    }
    void set_text(std::size_t index, std::string value)
    {
        // 字符串不能保存容器引用，写入时无需登记循环回收器。
        elements_->values[index] = std::move(value);
    }
    void set_value(std::size_t index, const std::any& value)
    {
        if (may_contain_cycle(value))
        {
            register_storage();
        }
        elements_->values[index] = value;
    }
    [[nodiscard]] const void* identity() const noexcept
    {
        return elements_.get();
    }

private:
    struct storage
    {
        std::vector<std::any> values;
        bool registered = false;

        storage() = default;
        explicit storage(std::size_t count) : values(count) {}
        template<class input_iterator>
        storage(input_iterator first, input_iterator last) : values(first, last) {}
    };

    static bool may_contain_cycle(const std::any& value)
    {
        return value.has_value() &&
               value.type() != typeid(std::int64_t) &&
               value.type() != typeid(double) &&
               value.type() != typeid(bool) &&
               value.type() != typeid(std::string);
    }

    void register_storage()
    {
        if (elements_->registered)
        {
            return;
        }
        register_gc_node(elements_,
            [](const void* object, gc_visit visit, void* context)
            {
                for (const auto& value : static_cast<const storage*>(object)->values)
                {
                    visit(value, context);
                }
            },
            [](void* object)
            {
                static_cast<storage*>(object)->values.clear();
            });
        elements_->registered = true;
    }

    std::shared_ptr<storage> elements_;
};

} // namespace tx_generated
