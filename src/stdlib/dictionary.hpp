#pragma once

#include <any>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace tx_generated
{

class tx_dict
{
public:
    using entry = std::pair<std::any, std::any>;

    tx_dict();

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] std::any* find_value(const std::any& key);
    [[nodiscard]] std::any* find_value(std::string_view key);
    [[nodiscard]] const std::any* find_value(const std::any& key) const;
    [[nodiscard]] const std::any* find_value(std::string_view key) const;
    std::any& emplace_back(std::any key, std::any value);
    [[nodiscard]] bool erase(const std::any& key);
    void clear() noexcept;
    [[nodiscard]] const void* identity() const noexcept;

    template<class operation>
    void for_each(operation&& apply) const
    {
        for (const auto& [index, item] : data_->entries)
        {
            (void)index;
            apply(item.first, item.second);
        }
        for (const auto& item : data_->nan_entries)
        {
            apply(item.first, item.second);
        }
    }

private:
    using index_key = std::variant<std::monostate, std::int64_t, double,
                                   bool, std::string>;
    [[nodiscard]] static std::optional<index_key> make_key(const std::any& key);

    struct key_hash
    {
        using is_transparent = void;

        [[nodiscard]] std::size_t operator()(const index_key& key) const noexcept
        {
            std::size_t value = 0;
            switch (key.index())
            {
            case 1: value = std::hash<std::int64_t>{}(
                std::get<std::int64_t>(key)); break;
            case 2: value = std::hash<double>{}(std::get<double>(key)); break;
            case 3: value = std::hash<bool>{}(std::get<bool>(key)); break;
            case 4: value = std::hash<std::string_view>{}(
                std::get<std::string>(key)); break;
            default: break;
            }
            return mix(value, key.index());
        }

        [[nodiscard]] std::size_t operator()(std::string_view key) const noexcept
        {
            return mix(std::hash<std::string_view>{}(key), 4);
        }

    private:
        [[nodiscard]] static std::size_t mix(std::size_t value,
                                              std::size_t kind) noexcept
        {
            return value ^ (kind * static_cast<std::size_t>(
                0x9e3779b97f4a7c15ULL));
        }
    };

    struct key_equal
    {
        using is_transparent = void;

        [[nodiscard]] bool operator()(const index_key& left,
                                       const index_key& right) const noexcept
        {
            return left == right;
        }

        [[nodiscard]] bool operator()(const index_key& left,
                                       std::string_view right) const noexcept
        {
            const auto* text = std::get_if<std::string>(&left);
            return text != nullptr && *text == right;
        }

        [[nodiscard]] bool operator()(std::string_view left,
                                       const index_key& right) const noexcept
        {
            return (*this)(right, left);
        }
    };

    // NaN 不满足哈希表键要求的自反相等性，单独保留且查找永不命中。
    struct storage
    {
        std::unordered_map<index_key, entry, key_hash, key_equal> entries;
        std::vector<entry> nan_entries;
    };
    std::shared_ptr<storage> data_;
};

} // namespace tx_generated
