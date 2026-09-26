#pragma once

#include "backend/cpp/cycle_gc.hpp"

#include <any>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>

namespace tx_generated
{

struct iterator_state
{
    std::any values;
    std::string element_type;
    std::size_t index = 0;
    bool exhausted = false;
    bool closed = false;
};

class tx_iterator
{
public:
    explicit tx_iterator(iterator_state state)
        : state_(std::make_shared<iterator_state>(std::move(state)))
    {
        register_gc_node(state_,
            [](const void* value, gc_visit visit, void* context)
            {
                visit(static_cast<const iterator_state*>(value)->values, context);
            },
            [](void* value)
            {
                static_cast<iterator_state*>(value)->values.reset();
            });
    }

    [[nodiscard]] iterator_state& data() const noexcept
    {
        return *state_;
    }

    [[nodiscard]] const void* identity() const noexcept
    {
        return state_.get();
    }

private:
    std::shared_ptr<iterator_state> state_;
};

} // namespace tx_generated
