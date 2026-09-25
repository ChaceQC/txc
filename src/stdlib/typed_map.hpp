#pragma once

#include "stdlib/container_scalar.hpp"
#include "stdlib/typed_container.hpp"
#include "stdlib/vector.hpp"

#include <unordered_map>
#include <unordered_set>

namespace tx_generated
{

template<class key_type, class value_type>
struct map_storage final : container_model<map_storage<key_type, value_type>>
{
    std::unordered_map<key_type, value_type, scalar_hash<key_type>, scalar_equal<key_type>> values;

    [[nodiscard]] std::string type_name() const override
    {
        return "map<" + scalar_name<key_type>() + "," + scalar_name<value_type>() + ">";
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
        tx_vector<std::conditional_t<keys, key_type, value_type>> result;
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
