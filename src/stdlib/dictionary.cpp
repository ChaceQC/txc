#include "stdlib/dictionary.hpp"

#include "backend/cpp/cycle_gc.hpp"

#include <cmath>
#include <stdexcept>

namespace tx_generated
{

tx_dict::tx_dict() : data_(std::make_shared<storage>())
{
    register_gc_node(data_,
        [](const void* object, gc_visit visit, void* context)
        {
            for (const auto& [key, value] :
                 static_cast<const storage*>(object)->entries)
            {
                visit(key, context);
                visit(value, context);
            }
        },
        [](void* object)
        {
            auto& data = *static_cast<storage*>(object);
            data.entries.clear();
            data.positions.clear();
        });
}

const void* tx_dict::identity() const noexcept
{
    return data_.get();
}

std::size_t tx_dict::size() const noexcept
{
    return data_->entries.size();
}

tx_dict::entry& tx_dict::operator[](std::size_t index)
{
    return data_->entries[index];
}

const tx_dict::entry& tx_dict::operator[](std::size_t index) const
{
    return data_->entries[index];
}

tx_dict::entry& tx_dict::back()
{
    return data_->entries.back();
}

tx_dict::iterator tx_dict::begin() noexcept
{
    return data_->entries.begin();
}

tx_dict::iterator tx_dict::end() noexcept
{
    return data_->entries.end();
}

tx_dict::const_iterator tx_dict::begin() const noexcept
{
    return data_->entries.begin();
}

tx_dict::const_iterator tx_dict::end() const noexcept
{
    return data_->entries.end();
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
    const auto found = data_->positions.find(*index);
    return found == data_->positions.end() ? nullptr
                                           : &data_->entries[found->second].second;
}

std::any* tx_dict::find_value(std::string_view key)
{
    const index_key index = std::string(key);
    const auto found = data_->positions.find(index);
    return found == data_->positions.end() ? nullptr
                                           : &data_->entries[found->second].second;
}

void tx_dict::emplace_back(std::any key, std::any value)
{
    const auto index = make_key(key);
    data_->entries.emplace_back(std::move(key), std::move(value));
    if (index)
    {
        try
        {
            data_->positions.emplace(*index, data_->entries.size() - 1);
        }
        catch (...)
        {
            data_->entries.pop_back();
            throw;
        }
    }
}

} // namespace tx_generated
