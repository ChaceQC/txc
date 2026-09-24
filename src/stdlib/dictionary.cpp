#include "stdlib/dictionary.hpp"

#include <cmath>
#include <stdexcept>

namespace tx_generated
{

std::size_t tx_dict::size() const noexcept
{
    return entries_.size();
}

tx_dict::entry& tx_dict::operator[](std::size_t index)
{
    return entries_[index];
}

const tx_dict::entry& tx_dict::operator[](std::size_t index) const
{
    return entries_[index];
}

tx_dict::entry& tx_dict::back()
{
    return entries_.back();
}

tx_dict::iterator tx_dict::begin() noexcept
{
    return entries_.begin();
}

tx_dict::iterator tx_dict::end() noexcept
{
    return entries_.end();
}

tx_dict::const_iterator tx_dict::begin() const noexcept
{
    return entries_.begin();
}

tx_dict::const_iterator tx_dict::end() const noexcept
{
    return entries_.end();
}

std::optional<tx_dict::index_key> tx_dict::make_key(const std::any& key)
{
    if (!key.has_value())
    {
        return std::monostate{};
    }
    if (key.type() == typeid(std::int64_t))
    {
        return std::any_cast<std::int64_t>(key);
    }
    if (key.type() == typeid(double))
    {
        const double value = std::any_cast<double>(key);
        // 原有语义中 NaN 与自身也不相等，不能放入要求等价关系的哈希索引。
        return std::isnan(value) ? std::nullopt
                                 : std::optional<index_key>{value};
    }
    if (key.type() == typeid(bool))
    {
        return std::any_cast<bool>(key);
    }
    if (key.type() == typeid(std::string))
    {
        return std::any_cast<const std::string&>(key);
    }
    throw std::runtime_error("字典键必须是可哈希的值");
}

std::any* tx_dict::find_value(const std::any& key)
{
    const auto index = make_key(key);
    if (!index)
    {
        return nullptr;
    }
    const auto found = positions_.find(*index);
    return found == positions_.end() ? nullptr
                                     : &entries_[found->second].second;
}

void tx_dict::emplace_back(std::any key, std::any value)
{
    const auto index = make_key(key);
    entries_.emplace_back(std::move(key), std::move(value));
    if (index)
    {
        try
        {
            positions_.emplace(*index, entries_.size() - 1);
        }
        catch (...)
        {
            entries_.pop_back();
            throw;
        }
    }
}

} // namespace tx_generated
