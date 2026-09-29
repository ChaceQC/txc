#pragma once

#include "backend/cpp/cycle_gc.hpp"
#include "backend/cpp/typed_slots.hpp"

#include <any>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace tx_generated
{

struct closure_view
{
    const void* code = nullptr;
    const closure_view* parent = nullptr;
    const typed_slot* captures = nullptr;
};

static_assert(sizeof(closure_view) == 24);

struct closure_state
{
    const void* target = nullptr;
    std::string type_name;
    std::any parent;
    std::vector<std::any> captures;
    closure_view view = {};
    typed_slots typed_captures = {};

    void refresh() noexcept;
};

class closure_handle
{
public:
    explicit closure_handle(closure_state state)
        : state_(std::make_shared<closure_state>(std::move(state)))
    {
        state_->refresh();
        register_gc_node(state_,
            [](const void* value, gc_visit visit, void* context)
            {
                const auto& state = *static_cast<const closure_state*>(value);
                visit(state.parent, context);
                for (const auto& capture : state.captures)
                {
                    visit(capture, context);
                }
                for (std::size_t index = 0; index < state.typed_captures.size(); ++index)
                {
                    if (state.typed_captures.kind(index) == slot_kind::reference)
                    {
                        visit(state.typed_captures.reference(index), context);
                    }
                }
            },
            [](void* value)
            {
                auto& state = *static_cast<closure_state*>(value);
                state.parent.reset();
                state.captures.clear();
                state.typed_captures.clear();
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

inline void closure_state::refresh() noexcept
{
    const auto* closure = std::any_cast<closure_handle>(&parent);
    view.parent = closure ? &closure->data().view : nullptr;
    view.captures = typed_captures.data();
}

} // namespace tx_generated
