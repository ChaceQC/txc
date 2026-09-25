#pragma once

#include "backend/cpp/cycle_gc.hpp"
#include "backend/cpp/text_reference.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
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
        std::vector<element_type> values;

        void refresh() noexcept
        {
            view = {values.data(), values.size(), values.capacity()};
        }
    };

    tx_vector() : data_(std::make_shared<storage>())
    {
        note_gc_allocation();
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
        tx_vector result;
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

} // namespace tx_generated
