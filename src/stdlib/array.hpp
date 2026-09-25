#pragma once

#include "backend/cpp/cycle_gc.hpp"

#include <any>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace tx_generated
{

class tx_array
{
public:
    using storage = std::vector<std::any>;
    using iterator = storage::iterator;
    using const_iterator = storage::const_iterator;

    tx_array() : elements_(std::make_shared<storage>())
    {
        register_storage();
    }
    explicit tx_array(std::size_t count)
        : elements_(std::make_shared<storage>(count))
    {
        register_storage();
    }

    template<class input_iterator>
    tx_array(input_iterator first, input_iterator last)
        : elements_(std::make_shared<storage>(first, last))
    {
        register_storage();
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return elements_->size();
    }
    [[nodiscard]] std::any& operator[](std::size_t index)
    {
        return (*elements_)[index];
    }
    [[nodiscard]] const std::any& operator[](std::size_t index) const
    {
        return (*elements_)[index];
    }
    [[nodiscard]] iterator begin() noexcept
    {
        return elements_->begin();
    }
    [[nodiscard]] iterator end() noexcept
    {
        return elements_->end();
    }
    [[nodiscard]] const_iterator begin() const noexcept
    {
        return elements_->begin();
    }
    [[nodiscard]] const_iterator end() const noexcept
    {
        return elements_->end();
    }
    void reserve(std::size_t count)
    {
        elements_->reserve(count);
    }
    void push_back(std::any value)
    {
        elements_->push_back(std::move(value));
    }
    template<class... arguments>
    void emplace_back(arguments&&... values)
    {
        elements_->emplace_back(std::forward<arguments>(values)...);
    }
    void insert(iterator where, const_iterator first, const_iterator last)
    {
        elements_->insert(where, first, last);
    }
    [[nodiscard]] const void* identity() const noexcept
    {
        return elements_.get();
    }

private:
    void register_storage()
    {
        register_gc_node(elements_,
            [](const void* object, gc_visit visit, void* context)
            {
                for (const auto& value : *static_cast<const storage*>(object))
                {
                    visit(value, context);
                }
            },
            [](void* object)
            {
                static_cast<storage*>(object)->clear();
            });
    }

    std::shared_ptr<storage> elements_;
};

} // namespace tx_generated
