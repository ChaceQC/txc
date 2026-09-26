#pragma once

#include "stdlib/typed_container.hpp"
#include "stdlib/container_scalar.hpp"
#include "stdlib/vector.hpp"

#include <any>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace tx_generated
{

using key_hash = std::function<std::size_t(const std::any&)>;
using key_equal = std::function<bool(const std::any&, const std::any&)>;
using key_copy = std::function<std::any(const std::any&)>;

struct object_hash
{
    key_hash callback;

    [[nodiscard]] std::size_t operator()(const std::any& value) const
    {
        return callback(value);
    }
};

struct object_equal
{
    key_equal callback;

    [[nodiscard]] bool operator()(const std::any& left,
                                  const std::any& right) const
    {
        return callback(left, right);
    }
};

template<class value_type>
struct object_key_map final : container_model<object_key_map<value_type>>
{
    using entries_type = std::unordered_map<std::any, value_type,
        object_hash, object_equal>;

    object_key_map(std::string key_type, key_hash hash, key_equal equal,
                   key_copy copy)
        : key_type(std::move(key_type)), copy_key(std::move(copy)),
          values(0, object_hash{std::move(hash)},
                 object_equal{std::move(equal)})
    {
    }

    [[nodiscard]] std::string type_name() const override
    {
        return "map<" + key_type + "," + scalar_name<value_type>() + ">";
    }

    void set(const std::any& key, const value_type& value)
    {
        // 插入的是脱离调用方对象图的键，后续原键赋值不会破坏哈希表。
        values.insert_or_assign(copy_key(key), value);
    }

    [[nodiscard]] value_type read(const std::any& key) const
    {
        const auto found = values.find(key);
        if (found == values.end())
        {
            throw std::out_of_range("map 键不存在");
        }
        return found->second;
    }

    [[nodiscard]] value_type get(const std::any& key,
                                 const value_type& fallback) const
    {
        const auto found = values.find(key);
        return found == values.end() ? fallback : found->second;
    }

    [[nodiscard]] object_vector keys() const
    {
        object_vector result(key_type);
        result.data().values.reserve(values.size());
        for (const auto& [key, value] : values)
        {
            (void)value;
            result.data().values.push_back(copy_key(key));
        }
        result.data().refresh();
        return result;
    }

    [[nodiscard]] tx_vector<value_type> values_snapshot() const
    {
        tx_vector<value_type> result;
        result.data().values.reserve(values.size());
        for (const auto& [key, value] : values)
        {
            (void)key;
            result.data().values.push_back(value);
        }
        result.data().refresh();
        return result;
    }

    [[nodiscard]] std::string repr() const override
    {
        std::string result = type_name() + "{";
        bool first = true;
        for (const auto& [key, value] : values)
        {
            result += first ? "" : ", ";
            first = false;
            result += format_repr_value(key) + ": " +
                format_repr_value(scalar_value(value));
        }
        return result + "}";
    }

    std::string key_type;
    key_copy copy_key;
    entries_type values;
};

struct object_key_set final : container_model<object_key_set>
{
    using entries_type = std::unordered_set<std::any,
        object_hash, object_equal>;

    object_key_set(std::string key_type, key_hash hash, key_equal equal,
                   key_copy copy)
        : key_type(std::move(key_type)), copy_key(std::move(copy)),
          values(0, object_hash{std::move(hash)},
                 object_equal{std::move(equal)})
    {
    }

    [[nodiscard]] std::string type_name() const override
    {
        return "set<" + key_type + ">";
    }

    [[nodiscard]] bool insert(const std::any& key)
    {
        return values.insert(copy_key(key)).second;
    }

    [[nodiscard]] object_vector snapshot() const
    {
        object_vector result(key_type);
        result.data().values.reserve(values.size());
        for (const auto& key : values)
        {
            result.data().values.push_back(copy_key(key));
        }
        result.data().refresh();
        return result;
    }

    [[nodiscard]] std::string repr() const override
    {
        std::string result = type_name() + "{";
        bool first = true;
        for (const auto& key : values)
        {
            result += first ? "" : ", ";
            first = false;
            result += format_repr_value(key);
        }
        return result + "}";
    }

    std::string key_type;
    key_copy copy_key;
    entries_type values;
};

} // namespace tx_generated
