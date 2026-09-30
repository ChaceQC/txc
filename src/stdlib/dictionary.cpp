#include "stdlib/dictionary.hpp"
#include "stdlib/stdlib.hpp"

#include "backend/cpp/cycle_gc.hpp"

#include <cmath>
#include <stdexcept>

namespace tx_generated
{

tx_dict::tx_dict() : data_(std::make_shared<storage>())
{
    note_gc_allocation();
}

void tx_dict::register_storage()
{
    if (data_->registered)
    {
        return;
    }
    register_gc_node(data_,
        [](const void* object, gc_visit visit, void* context)
        {
            const auto& data = *static_cast<const storage*>(object);
            for (const auto& [index, item] : data.entries)
            {
                (void)index;
                visit(item.first, context);
                visit(item.second, context);
            }
            for (const auto& item : data.nan_entries)
            {
                visit(item.first, context);
                visit(item.second, context);
            }
        },
        [](void* object)
        {
            auto& data = *static_cast<storage*>(object);
            data.entries.clear();
            data.nan_entries.clear();
        });
    data_->registered = true;
}

void tx_dict::prepare_write()
{
    // 通用可写地址无法在返回时得知未来写入的类型，必须提前登记。
    register_storage();
}

void tx_dict::reserve(std::size_t count)
{
    data_->entries.reserve(count);
}

const void* tx_dict::identity() const noexcept
{
    return data_.get();
}

std::size_t tx_dict::size() const noexcept
{
    return data_->entries.size() + data_->nan_entries.size();
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
    return const_cast<std::any*>(std::as_const(*this).find_value(key));
}

const std::any* tx_dict::find_value(const std::any& key) const
{
    const auto index = make_key(key);
    if (!index)
    {
        return nullptr;
    }
    const auto found = data_->entries.find(*index);
    return found == data_->entries.end() ? nullptr : &found->second.second;
}

std::any* tx_dict::find_value(std::string_view key)
{
    return const_cast<std::any*>(std::as_const(*this).find_value(key));
}

const std::any* tx_dict::find_value(std::string_view key) const
{
    const auto found = data_->entries.find(key);
    return found == data_->entries.end() ? nullptr : &found->second.second;
}

std::any& tx_dict::emplace_back(std::any key, std::any value)
{
    if (value.has_value() && value.type() != typeid(std::int64_t) &&
        value.type() != typeid(double) && value.type() != typeid(bool) &&
        value.type() != typeid(std::string))
    {
        register_storage();
    }
    const auto index = make_key(key);
    if (!index)
    {
        data_->nan_entries.emplace_back(std::move(key), std::move(value));
        return data_->nan_entries.back().second;
    }
    const auto found = data_->entries.find(*index);
    if (found != data_->entries.end())
    {
        found->second.second = std::move(value);
        return found->second.second;
    }
    const auto [inserted, success] = data_->entries.emplace(
        *index, entry{std::move(key), std::move(value)});
    (void)success;
    return inserted->second.second;
}

bool tx_dict::erase(const std::any& key)
{
    const auto index = make_key(key);
    if (!index)
    {
        return false;
    }
    return data_->entries.erase(*index) != 0;
}

bool tx_dict::erase(std::string_view key)
{
    const auto found = data_->entries.find(key);
    if (found == data_->entries.end())
    {
        return false;
    }
    data_->entries.erase(found);
    return true;
}

void tx_dict::clear() noexcept
{
    data_->entries.clear();
    data_->nan_entries.clear();
}

std::any tx_fn_dictionary_get(const tx_dict& values, const std::any& key)
{
    const auto* found = values.find_value(key);
    return found ? *found : std::any{};
}

bool tx_fn_dictionary_contains(const tx_dict& values, const std::any& key)
{
    return values.find_value(key) != nullptr;
}

bool tx_fn_dictionary_remove(tx_dict values, const std::any& key)
{
    return values.erase(key);
}

tx_array tx_fn_dictionary_keys(const tx_dict& values)
{
    tx_array result;
    result.reserve(values.size());
    values.for_each([&](const std::any& key, const std::any& value)
    {
        (void)value;
        result.push_back(key);
    });
    return result;
}

tx_array tx_fn_dictionary_values(const tx_dict& values)
{
    tx_array result;
    result.reserve(values.size());
    values.for_each([&](const std::any& key, const std::any& value)
    {
        (void)key;
        result.push_back(value);
    });
    return result;
}

void tx_fn_dictionary_clear(tx_dict values) noexcept
{
    values.clear();
}

tx_array tx_fn_dictionary_items(const tx_dict& values)
{
    tx_array result;
    result.reserve(values.size());
    values.for_each([&](const std::any& key, const std::any& value)
    {
        tx_array pair;
        pair.reserve(2);
        pair.push_back(key);
        pair.push_back(value);
        result.push_back(std::move(pair));
    });
    return result;
}

} // namespace tx_generated
