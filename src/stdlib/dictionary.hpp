#pragma once

#include <any>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>

namespace tx_generated
{

class tx_dict
{
public:
    using entry = std::pair<std::any, std::any>;
    using iterator = std::deque<entry>::iterator;
    using const_iterator = std::deque<entry>::const_iterator;

    tx_dict();

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] entry& operator[](std::size_t index);
    [[nodiscard]] const entry& operator[](std::size_t index) const;
    [[nodiscard]] entry& back();
    [[nodiscard]] iterator begin() noexcept;
    [[nodiscard]] iterator end() noexcept;
    [[nodiscard]] const_iterator begin() const noexcept;
    [[nodiscard]] const_iterator end() const noexcept;
    [[nodiscard]] std::any* find_value(const std::any& key);
    [[nodiscard]] std::any* find_value(std::string_view key);
    void emplace_back(std::any key, std::any value);
    [[nodiscard]] const void* identity() const noexcept;

private:
    using index_key = std::variant<std::monostate, std::int64_t, double,
                                   bool, std::string>;
    [[nodiscard]] static std::optional<index_key> make_key(const std::any& key);

    // 顺序表保持遍历顺序；索引只记录非 NaN 键的位置。
    struct storage
    {
        std::deque<entry> entries;
        std::unordered_map<index_key, std::size_t> positions;
    };
    std::shared_ptr<storage> data_;
};

} // namespace tx_generated
