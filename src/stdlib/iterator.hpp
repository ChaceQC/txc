#pragma once

#include "backend/cpp/cycle_gc.hpp"

#include <any>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace tx_generated
{

// 独立于 std::any / vector 的稳定 ABI；next 必须逐次写回 index。
struct iterator_cursor
{
    const void* elements = nullptr;
    std::uint64_t size = 0;
    std::uint64_t index = 0;
    bool exhausted = false;
    bool closed = false;
};

static_assert(sizeof(iterator_cursor) == 32 && offsetof(iterator_cursor, closed) == 25);

struct iterator_state
{
    std::any values;
    std::string element_type;
    iterator_cursor cursor;
};

class tx_iterator
{
public:
    explicit tx_iterator(iterator_state state)
        : state_(std::make_shared<iterator_state>(std::move(state)))
    {
        // 标量/文本向量不含对象边，不可能与游标成环。
        if (state_->element_type == "int" || state_->element_type == "float" ||
            state_->element_type == "bool" || state_->element_type == "str" ||
            state_->element_type == "bytes")
        {
            note_gc_allocation();
            return;
        }
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
