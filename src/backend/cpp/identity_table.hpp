#pragma once

#include <array>
#include <algorithm>
#include <cstddef>
#include <bit>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include <utility>

namespace tx_generated
{

// 小图不分配；大图使用连续开放寻址表，避免每个对象各分配一个哈希节点。
template<class value_type, std::size_t capacity = 8>
class identity_table
{
public:
    struct entry
    {
        const void* first = nullptr;
        value_type second{};
    };

    const entry* find(const void* key) const
    {
        if (large_)
        {
            if (!key)
            {
                return nullptr;
            }
            const auto& found = values_[position(values_, key)];
            return found.first ? &found : nullptr;
        }
        for (std::size_t index = 0; index < size_; ++index)
        {
            if (local_[index].first == key)
            {
                return &local_[index];
            }
        }
        return nullptr;
    }

    const entry* end() const noexcept
    {
        return nullptr;
    }

    void reserve(std::size_t count)
    {
        if (count > capacity)
        {
            grow(count);
        }
    }

    void clear() noexcept
    {
        for (auto& item : local_)
        {
            item = {};
        }
        for (auto& item : values_)
        {
            item = {};
        }
        size_ = 0;
    }

    template<class source_type>
    void emplace(const void* key, source_type&& value)
    {
        if ((!large_ && size_ == capacity) || (large_ && size_ >= values_.size() / 2))
        {
            grow(size_ * 2);
        }
        if (large_)
        {
            auto& destination = values_[position(values_, key)];
            if (!destination.first)
            {
                destination = entry{key, std::forward<source_type>(value)};
                ++size_;
            }
        }
        else
        {
            local_[size_++] = entry{key, std::forward<source_type>(value)};
        }
    }

private:
    static std::size_t position(const std::vector<entry>& values, const void* key) noexcept
    {
        auto hash = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(key));
        hash ^= hash >> 33;
        hash *= 0xff51afd7ed558ccdULL;
        hash ^= hash >> 33;
        auto index = static_cast<std::size_t>(hash) & (values.size() - 1);
        while (values[index].first && values[index].first != key)
        {
            index = (index + 1) & (values.size() - 1);
        }
        return index;
    }

    void grow(std::size_t count)
    {
        if (large_ && count <= values_.size() / 2)
        {
            return;
        }
        if (count > std::numeric_limits<std::size_t>::max() / 4)
        {
            throw std::length_error("对象身份表过大");
        }
        std::vector<entry> expanded(std::bit_ceil(std::max(count * 2, std::size_t{2})));
        static_assert(std::is_nothrow_move_assignable_v<entry>);
        const auto insert = [&](entry& item)
        {
            if (item.first)
            {
                expanded[position(expanded, item.first)] = std::move(item);
            }
        };
        if (large_)
        {
            for (auto& item : values_)
            {
                insert(item);
            }
        }
        else
        {
            // 整块分配成功后才无异常移动，避免重哈希再次分配 any 内的对象句柄。
            for (std::size_t index = 0; index < size_; ++index)
            {
                insert(local_[index]);
            }
            for (std::size_t index = 0; index < size_; ++index)
            {
                local_[index] = {};
            }
        }
        values_.swap(expanded);
        large_ = true;
    }

    std::array<entry, capacity> local_{};
    std::size_t size_ = 0;
    bool large_ = false;
    std::vector<entry> values_;
};

} // namespace tx_generated
