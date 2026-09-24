#pragma once

#include <any>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
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
    using iterator = std::vector<entry>::iterator;
    using const_iterator = std::vector<entry>::const_iterator;

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] entry& operator[](std::size_t index);
    [[nodiscard]] const entry& operator[](std::size_t index) const;
    [[nodiscard]] entry& back();
    [[nodiscard]] iterator begin() noexcept;
    [[nodiscard]] iterator end() noexcept;
    [[nodiscard]] const_iterator begin() const noexcept;
    [[nodiscard]] const_iterator end() const noexcept;
    [[nodiscard]] std::any* find_value(const std::any& key);
    void emplace_back(std::any key, std::any value);

private:
    using index_key = std::variant<std::monostate, std::int64_t, double,
                                   bool, std::string>;
    [[nodiscard]] static std::optional<index_key> make_key(const std::any& key);

    // 顺序表保持遍历顺序；索引只记录非 NaN 键的位置。
    std::vector<entry> entries_;
    std::unordered_map<index_key, std::size_t> positions_;
};

} // namespace tx_generated
