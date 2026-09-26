#pragma once

#include "backend/cpp/cycle_gc.hpp"
#include "backend/cpp/text_reference.hpp"

#include <cstddef>
#include <cstdint>
#include <any>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace tx_generated
{

// TX 自有、稳定的只读布局视图；不向 LLVM 暴露 std::vector 内部字段。
struct vector_view
{
    void* data = nullptr;
    std::uint64_t size = 0;
    std::uint64_t capacity = 0;
};

static_assert(offsetof(vector_view, data) == 0 &&
              offsetof(vector_view, size) == 8 &&
              offsetof(vector_view, capacity) == 16);

template<class element_type>
class tx_vector
{
public:
    struct storage
    {
        vector_view view;
        std::string type_name;
        std::vector<element_type> values;

        void refresh() noexcept
        {
            view = {values.data(), values.size(), values.capacity()};
        }
    };

    explicit tx_vector(std::string type_name = {}) : data_(std::make_shared<storage>())
    {
        data_->type_name = std::move(type_name);
        if constexpr (std::is_same_v<element_type, std::any>)
        {
            // 复合元素可能指回本向量，必须进入与 array/class 相同的对象图。
            register_gc_node(data_,
                [](const void* value, gc_visit visit, void* context)
                {
                    for (const auto& item : static_cast<const storage*>(value)->values)
                    {
                        visit(item, context);
                    }
                },
                [](void* value)
                {
                    static_cast<storage*>(value)->values.clear();
                });
        }
        else
        {
            note_gc_allocation();
        }
    }
    [[nodiscard]] storage& data() const noexcept
    {
        return *data_;
    }
    [[nodiscard]] const void* identity() const noexcept
    {
        return data_.get();
    }
    [[nodiscard]] tx_vector copy() const
    {
        tx_vector result(data_->type_name);
        result.data().values = data_->values;
        result.data().refresh();
        return result;
    }

private:
    std::shared_ptr<storage> data_;
};

using int_vector = tx_vector<std::int64_t>;
using float_vector = tx_vector<double>;
using bool_vector = tx_vector<std::uint8_t>;
using string_vector = tx_vector<text_reference>;
using byte_value = std::shared_ptr<const std::vector<std::uint8_t>>;
using bytes_vector = tx_vector<byte_value>;
using object_vector = tx_vector<std::any>;

} // namespace tx_generated
