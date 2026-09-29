#pragma once

#include <array>
#include <cstddef>
#include <unordered_map>
#include <utility>

namespace tx_generated
{

// 小图不分配哈希节点；溢出时迁移一次，随后仍按对象身份查找。
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
            const auto found = values_.find(key);
            return found == values_.end() ? nullptr : &found->second;
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

    template<class source_type>
    void emplace(const void* key, source_type&& value)
    {
        if (!large_ && size_ == capacity)
        {
            grow(capacity * 2);
        }
        if (large_)
        {
            values_.try_emplace(key, entry{key, std::forward<source_type>(value)});
        }
        else
        {
            local_[size_++] = entry{key, std::forward<source_type>(value)};
        }
    }

private:
    void grow(std::size_t count)
    {
        values_.reserve(count);
        if (!large_)
        {
            // 迁移期间保留原表，分配失败仍由上下文正常清理已复制的对象图。
            for (std::size_t index = 0; index < size_; ++index)
            {
                values_.emplace(local_[index].first, local_[index]);
            }
            for (std::size_t index = 0; index < size_; ++index)
            {
                local_[index] = {};
            }
            size_ = 0;
            large_ = true;
        }
    }

    std::array<entry, capacity> local_{};
    std::size_t size_ = 0;
    bool large_ = false;
    std::unordered_map<const void*, entry> values_;
};

} // namespace tx_generated
