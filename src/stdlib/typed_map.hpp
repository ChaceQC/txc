#pragma once

#include "stdlib/container_scalar.hpp"
#include "stdlib/map_entry.hpp"
#include "stdlib/typed_container.hpp"
#include "stdlib/vector.hpp"

#include <unordered_map>
#include <unordered_set>
#include <type_traits>
#include <memory>

namespace tx_generated
{

template<class key_type, class value_type>
struct map_storage;

template<class key_type, class value_type>
void register_map_storage(
    const std::shared_ptr<map_storage<key_type, value_type>>& storage);

template<class key_type, class value_type>
struct map_storage final : container_model<map_storage<key_type, value_type>>,
                           graph_copyable
{
    std::unordered_map<key_type, value_type, scalar_hash<key_type>, scalar_equal<key_type>> values;
    std::string value_name;

    explicit map_storage(std::string name = {}) : value_name(std::move(name))
    {
    }

    [[nodiscard]] std::string type_name() const override
    {
        const auto value_type_name = std::is_same_v<value_type, std::any>
            ? value_name : scalar_name<value_type>();
        return "map<" + scalar_name<key_type>() + "," + value_type_name + ">";
    }

    void set(const key_type& key, const value_type& value)
    {
        require_ordered_key(key);
        values.insert_or_assign(key, value);
    }

    [[nodiscard]] value_type read(const key_type& key) const
    {
        require_ordered_key(key);
        const auto found = values.find(key);
        if (found == values.end())
        {
            throw std::out_of_range("map 键不存在");
        }
        return found->second;
    }

    [[nodiscard]] value_type get(const key_type& key, const value_type& fallback) const
    {
        require_ordered_key(key);
        const auto found = values.find(key);
        return found == values.end() ? fallback : found->second;
    }

    template<bool keys>
    [[nodiscard]] auto snapshot() const
    {
        tx_vector<std::conditional_t<keys, key_type, value_type>> result(
            keys ? std::string{} : value_name);
        result.data().values.reserve(values.size());
        for (const auto& [key, value] : values)
        {
            if constexpr (keys)
            {
                result.data().values.push_back(key);
            }
            else
            {
                result.data().values.push_back(value);
            }
        }
        result.data().refresh();
        return result;
    }

    [[nodiscard]] object_vector entries() const
    {
        const std::string name = "entry<" + scalar_name<key_type>() + "," +
            (std::is_same_v<value_type, std::any> ? value_name :
             scalar_name<value_type>()) + ">";
        object_vector result(name);
        result.data().values.reserve(values.size());
        for (const auto& [key, value] : values)
        {
            result.data().values.push_back(make_map_entry(name, key, value));
        }
        result.data().refresh();
        return result;
    }

    [[nodiscard]] container_handle empty_graph_copy() const override
    {
        auto result = std::make_shared<map_storage>(value_name);
        register_map_storage(result);
        return result;
    }

    void fill_graph_copy(container_storage& target,
                         const graph_copy_function& copy) const override
    {
        auto& destination = static_cast<map_storage&>(target).values;
        for (const auto& [key, value] : values)
        {
            if constexpr (std::is_same_v<value_type, std::any>)
            {
                destination.emplace(key, copy(value));
            }
            else
            {
                destination.emplace(key, value);
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
            result += format_repr_value(scalar_value(key)) + ": " +
                      format_repr_value(scalar_value(value));
        }
        return result + "}";
    }
};

template<class key_type, class value_type>
void register_map_storage(
    const std::shared_ptr<map_storage<key_type, value_type>>& storage)
{
    if constexpr (std::is_same_v<value_type, std::any>)
    {
        register_gc_node(storage,
            [](const void* object, gc_visit visit, void* context)
            {
                for (const auto& [key, value] :
                     static_cast<const map_storage<key_type, value_type>*>(object)->values)
                {
                    (void)key;
                    visit(value, context);
                }
            },
            [](void* object)
            {
                static_cast<map_storage<key_type, value_type>*>(object)->values.clear();
            });
    }
    else
    {
        note_gc_allocation();
    }
}

template<class element_type>
struct set_storage final : container_model<set_storage<element_type>>
{
    std::unordered_set<element_type, scalar_hash<element_type>, scalar_equal<element_type>> values;

    [[nodiscard]] std::string type_name() const override
    {
        return "set<" + scalar_name<element_type>() + ">";
    }

    [[nodiscard]] bool insert(const element_type& value)
    {
        require_ordered_key(value);
        return values.insert(value).second;
    }

    [[nodiscard]] tx_vector<element_type> snapshot() const
    {
        tx_vector<element_type> result;
        result.data().values.assign(values.begin(), values.end());
        result.data().refresh();
        return result;
    }

    [[nodiscard]] std::string repr() const override
    {
        std::string result = type_name() + "{";
        bool first = true;
        for (const auto& value : values)
        {
            result += first ? "" : ", ";
            first = false;
            result += format_repr_value(scalar_value(value));
        }
        return result + "}";
    }
};

} // namespace tx_generated
