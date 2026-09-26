#pragma once

#include "stdlib/map_entry.hpp"
#include "stdlib/ordered_compare.hpp"
#include "stdlib/typed_container.hpp"
#include "stdlib/vector.hpp"

#include <any>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace tx_generated
{

template<class storage_type>
void register_ordered_storage(const std::shared_ptr<storage_type>& storage);

template<class key_type, class value_type>
struct ordered_map_storage final : container_model<ordered_map_storage<key_type, value_type>>,
                                   graph_copyable
{
    using values_type = std::map<key_type, value_type, ordered_compare<key_type>>;

    std::string key_name;
    std::string value_name;
    values_type values;

    ordered_map_storage(std::string key_name, std::string value_name,
                        const void* less, std::any closure)
        : key_name(std::move(key_name)), value_name(std::move(value_name)),
          values(ordered_compare<key_type>(less, std::move(closure)))
    {
    }

    [[nodiscard]] std::string type_name() const override
    {
        return "ordered_map<" + key_name + "," + value_name + ">";
    }

    void require_ready() const
    {
        if (*values.key_comp().invoking)
        {
            throw std::runtime_error("有序容器比较器不能递归访问同一容器");
        }
    }

    void set(const key_type& key, const value_type& value)
    {
        require_ready();
        require_ordered_key(key);
        values.insert_or_assign(copy_ordered_key(key), value);
    }

    [[nodiscard]] value_type read(const key_type& key) const
    {
        require_ready();
        require_ordered_key(key);
        const auto found = values.find(key);
        if (found == values.end())
        {
            throw std::out_of_range("ordered_map 键不存在");
        }
        return found->second;
    }

    [[nodiscard]] value_type get(const key_type& key,
                                 const value_type& fallback) const
    {
        require_ready();
        require_ordered_key(key);
        const auto found = values.find(key);
        return found == values.end() ? fallback : found->second;
    }

    [[nodiscard]] tx_vector<key_type> keys() const
    {
        require_ready();
        tx_vector<key_type> result(key_name);
        result.data().values.reserve(values.size());
        for (const auto& [key, value] : values)
        {
            (void)value;
            result.data().values.push_back(copy_ordered_key(key));
        }
        result.data().refresh();
        return result;
    }

    [[nodiscard]] tx_vector<value_type> values_snapshot() const
    {
        require_ready();
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
        require_ready();
        const auto entry_name = "entry<" + key_name + "," + value_name + ">";
        object_vector result(entry_name);
        result.data().values.reserve(values.size());
        for (const auto& [key, value] : values)
        {
            result.data().values.push_back(make_map_entry(
                entry_name, copy_ordered_key(key), value));
        }
        result.data().refresh();
        return result;
    }

    [[nodiscard]] object_vector range(const key_type& low,
                                      const key_type& high) const
    {
        require_ready();
        require_ordered_key(low);
        require_ordered_key(high);
        const auto entry_name = "entry<" + key_name + "," + value_name + ">";
        object_vector result(entry_name);
        const auto less = values.key_comp();
        if (less(high, low))
        {
            return result;
        }
        for (auto item = values.lower_bound(low);
             item != values.end() && less(item->first, high); ++item)
        {
            result.data().values.push_back(make_map_entry(
                entry_name, copy_ordered_key(item->first), item->second));
        }
        result.data().refresh();
        return result;
    }

    [[nodiscard]] container_handle empty_graph_copy() const override
    {
        auto result = std::make_shared<ordered_map_storage>(key_name,
            value_name, values.key_comp().less, std::any{});
        register_ordered_storage(result);
        return result;
    }

    void fill_graph_copy(container_storage& target,
                         const graph_copy_function& copy) const override
    {
        auto& destination = static_cast<ordered_map_storage&>(target);
        const auto compare = values.key_comp();
        destination.values = values_type(ordered_compare<key_type>(
            compare.less, copy(compare.closure)));
        for (const auto& [key, value] : values)
        {
            const auto copied_key = [&]() -> key_type
            {
                if constexpr (std::is_same_v<key_type, std::any>)
                {
                    return copy(key);
                }
                else
                {
                    return key;
                }
            }();
            if constexpr (std::is_same_v<value_type, std::any>)
            {
                destination.values.emplace(copied_key, copy(value));
            }
            else
            {
                destination.values.emplace(copied_key, value);
            }
        }
    }

    [[nodiscard]] std::string repr() const override
    {
        std::string output = type_name() + "{";
        bool first = true;
        for (const auto& [key, value] : values)
        {
            output += first ? "" : ", ";
            first = false;
            output += format_repr_value(entry_field_value(key)) + ": " +
                format_repr_value(entry_field_value(value));
        }
        return output + "}";
    }
};

template<class key_type>
struct ordered_set_storage final : container_model<ordered_set_storage<key_type>>,
                                   graph_copyable
{
    using values_type = std::set<key_type, ordered_compare<key_type>>;

    std::string key_name;
    values_type values;

    ordered_set_storage(std::string key_name, const void* less,
                        std::any closure)
        : key_name(std::move(key_name)),
          values(ordered_compare<key_type>(less, std::move(closure)))
    {
    }

    [[nodiscard]] std::string type_name() const override
    {
        return "ordered_set<" + key_name + ">";
    }

    void require_ready() const
    {
        if (*values.key_comp().invoking)
        {
            throw std::runtime_error("有序容器比较器不能递归访问同一容器");
        }
    }

    [[nodiscard]] bool insert(const key_type& key)
    {
        require_ready();
        require_ordered_key(key);
        return values.insert(copy_ordered_key(key)).second;
    }

    [[nodiscard]] tx_vector<key_type> snapshot() const
    {
        require_ready();
        tx_vector<key_type> result(key_name);
        result.data().values.reserve(values.size());
        for (const auto& key : values)
        {
            result.data().values.push_back(copy_ordered_key(key));
        }
        result.data().refresh();
        return result;
    }

    [[nodiscard]] tx_vector<key_type> range(const key_type& low,
                                            const key_type& high) const
    {
        require_ready();
        require_ordered_key(low);
        require_ordered_key(high);
        tx_vector<key_type> result(key_name);
        const auto less = values.key_comp();
        if (less(high, low))
        {
            return result;
        }
        for (auto item = values.lower_bound(low);
             item != values.end() && less(*item, high); ++item)
        {
            result.data().values.push_back(copy_ordered_key(*item));
        }
        result.data().refresh();
        return result;
    }

    [[nodiscard]] container_handle empty_graph_copy() const override
    {
        auto result = std::make_shared<ordered_set_storage>(key_name,
            values.key_comp().less, std::any{});
        register_ordered_storage(result);
        return result;
    }

    void fill_graph_copy(container_storage& target,
                         const graph_copy_function& copy) const override
    {
        auto& destination = static_cast<ordered_set_storage&>(target);
        const auto compare = values.key_comp();
        destination.values = values_type(ordered_compare<key_type>(
            compare.less, copy(compare.closure)));
        for (const auto& key : values)
        {
            if constexpr (std::is_same_v<key_type, std::any>)
            {
                destination.values.insert(copy(key));
            }
            else
            {
                destination.values.insert(key);
            }
        }
    }

    [[nodiscard]] std::string repr() const override
    {
        std::string output = type_name() + "{";
        bool first = true;
        for (const auto& key : values)
        {
            output += first ? "" : ", ";
            first = false;
            output += format_repr_value(entry_field_value(key));
        }
        return output + "}";
    }
};

template<class storage_type>
void register_ordered_storage(const std::shared_ptr<storage_type>& storage)
{
    register_gc_node(storage,
        [](const void* object, gc_visit visit, void* context)
        {
            const auto& data = *static_cast<const storage_type*>(object);
            const auto compare = data.values.key_comp();
            visit(compare.closure, context);
            for (const auto& value : data.values)
            {
                if constexpr (requires { value.second; })
                {
                    if constexpr (std::is_same_v<
                        std::decay_t<decltype(value.second)>, std::any>)
                    {
                        visit(value.second, context);
                    }
                }
            }
        },
        [](void* object)
        {
            auto& data = *static_cast<storage_type*>(object);
            data.values.clear();
            data.values = typename storage_type::values_type{};
        });
}

} // namespace tx_generated
