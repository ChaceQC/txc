#pragma once

#include "backend/cpp/cycle_gc.hpp"

#include <any>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace tx_generated
{

struct closure_state
{
    const void* target = nullptr;
    std::string type_name;
    std::any parent;
    std::vector<std::any> captures;
};

class closure_handle
{
public:
    explicit closure_handle(closure_state state)
        : state_(std::make_shared<closure_state>(std::move(state)))
    {
        register_gc_node(state_,
            [](const void* value, gc_visit visit, void* context)
            {
                const auto& state = *static_cast<const closure_state*>(value);
                visit(state.parent, context);
                for (const auto& capture : state.captures)
                {
                    visit(capture, context);
                }
            },
            [](void* value)
            {
                auto& state = *static_cast<closure_state*>(value);
                state.parent.reset();
                state.captures.clear();
            });
    }

    [[nodiscard]] closure_state& data() const noexcept
    {
        return *state_;
    }

    [[nodiscard]] const void* identity() const noexcept
    {
        return state_.get();
    }

private:
    std::shared_ptr<closure_state> state_;
};

} // namespace tx_generated
