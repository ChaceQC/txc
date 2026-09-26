#pragma once

#include "stdlib/typed_container.hpp"
#include "stdlib/container_scalar.hpp"
#include "stdlib/map_entry.hpp"
#include "stdlib/vector.hpp"

#include <any>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <type_traits>

namespace tx_generated
{

template<class value_type>
struct object_key_map;

template<class value_type>
void register_object_key_map(
    const std::shared_ptr<object_key_map<value_type>>& storage);

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
struct object_key_map final : container_model<object_key_map<value_type>>,
                              graph_copyable
{
    using entries_type = std::unordered_map<std::any, value_type,
        object_hash, object_equal>;

    object_key_map(std::string key_type, key_hash hash, key_equal equal,
                   key_copy copy, std::string value_name = {})
        : key_type(std::move(key_type)), copy_key(std::move(copy)),
          value_name(std::move(value_name)),
          values(0, object_hash{std::move(hash)},
                 object_equal{std::move(equal)})
    {
    }

    [[nodiscard]] std::string type_name() const override
    {
        return "map<" + key_type + "," +
            (std::is_same_v<value_type, std::any> ? value_name :
             scalar_name<value_type>()) + ">";
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
        tx_vector<value_type> result(value_name);
        result.data().values.reserve(values.size());
        for (const auto& [key, value] : values)
        {
            (void)key;
            result.data().values.push_back(value);
        }
        result.data().refresh();
        return result;
    }

    [[nodiscard]] object_vector entries() const
    {
        const std::string name = "entry<" + key_type + "," +
            (std::is_same_v<value_type, std::any> ? value_name :
             scalar_name<value_type>()) + ">";
        object_vector result(name);
        result.data().values.reserve(values.size());
        for (const auto& [key, value] : values)
        {
            result.data().values.push_back(
                make_map_entry(name, copy_key(key), value));
        }
        result.data().refresh();
        return result;
    }

    [[nodiscard]] container_handle empty_graph_copy() const override
    {
        auto result = std::make_shared<object_key_map>(key_type,
            values.hash_function().callback, values.key_eq().callback,
            copy_key, value_name);
        register_object_key_map(result);
        return result;
    }

    void fill_graph_copy(container_storage& target,
                         const graph_copy_function& copy) const override
    {
        auto& destination = static_cast<object_key_map&>(target).values;
        for (const auto& [key, value] : values)
        {
            auto copied_key = copy(key);
            if constexpr (std::is_same_v<value_type, std::any>)
            {
                destination.emplace(std::move(copied_key), copy(value));
            }
            else
            {
                destination.emplace(std::move(copied_key), value);
            }
        }
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
    std::string value_name;
    entries_type values;
};

template<class value_type>
void register_object_key_map(
    const std::shared_ptr<object_key_map<value_type>>& storage)
{
    if constexpr (std::is_same_v<value_type, std::any>)
    {
        register_gc_node(storage,
            [](const void* object, gc_visit visit, void* context)
            {
                for (const auto& [key, value] :
                     static_cast<const object_key_map<value_type>*>(object)->values)
                {
                    (void)key;
                    visit(value, context);
                }
            },
            [](void* object)
            {
                static_cast<object_key_map<value_type>*>(object)->values.clear();
            });
    }
    else
    {
        note_gc_allocation();
    }
}

struct object_key_set final : container_model<object_key_set>, graph_copyable
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

    [[nodiscard]] container_handle empty_graph_copy() const override
    {
        auto result = std::make_shared<object_key_set>(key_type,
            values.hash_function().callback, values.key_eq().callback,
            copy_key);
        note_gc_allocation();
        return result;
    }

    void fill_graph_copy(container_storage& target,
                         const graph_copy_function& copy) const override
    {
        auto& destination = static_cast<object_key_set&>(target).values;
        for (const auto& key : values)
        {
            destination.insert(copy(key));
        }
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
