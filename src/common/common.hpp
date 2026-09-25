#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tx
{

struct source_pos
{
    std::size_t line = 1;
    std::size_t column = 1;
    std::string file;
};

struct value_type
{
    value_type() = default;
    explicit value_type(std::string name)
        : name(std::move(name))
    {
    }

    std::string name = "unknown";
    std::vector<value_type> parameters;

    [[nodiscard]] bool is_function() const noexcept
    {
        return name.starts_with("fn(") && !parameters.empty();
    }

    [[nodiscard]] bool is_inferred_function() const noexcept
    {
        return name == "fn" && parameters.empty();
    }

    [[nodiscard]] static value_type function_of(
        std::vector<value_type> arguments, value_type result)
    {
        std::string name = "fn(";
        for (std::size_t index = 0; index < arguments.size(); ++index)
        {
            name += (index == 0 ? "" : ",") + arguments[index].name;
        }
        name += ")->" + result.name;
        arguments.push_back(std::move(result));
        value_type type(std::move(name));
        type.parameters = std::move(arguments);
        return type;
    }

    [[nodiscard]] bool is_vector() const noexcept
    {
        return name.starts_with("vector<") && parameters.size() == 1;
    }

    [[nodiscard]] std::string container_name() const
    {
        return name.substr(0, name.find('<'));
    }

    [[nodiscard]] bool is_map() const noexcept
    {
        return name.starts_with("map<") && parameters.size() == 2;
    }

    [[nodiscard]] bool is_typed_container() const noexcept
    {
        return is_map() || (parameters.size() == 1 &&
            (name.starts_with("set<") || name.starts_with("heap<") ||
             name.starts_with("queue<")));
    }

    [[nodiscard]] static bool is_container_name(std::string_view name)
    {
        return name == "vector" || name == "map" || name == "set" ||
               name == "heap" || name == "queue";
    }

    [[nodiscard]] static value_type container_of(
        std::string name, std::vector<value_type> arguments)
    {
        name += "<";
        for (std::size_t index = 0; index < arguments.size(); ++index)
        {
            name += (index == 0 ? "" : ",") + arguments[index].name;
        }
        value_type result(name + ">");
        result.parameters = std::move(arguments);
        return result;
    }

    [[nodiscard]] static value_type vector_of(value_type element)
    {
        value_type result("vector<" + element.name + ">");
        result.parameters.push_back(std::move(element));
        return result;
    }

    static const value_type int_type;
    static const value_type bool_type;
    static const value_type float_type;
    static const value_type str_type;
    static const value_type bytes_type;
    static const value_type binary_stream_type;
    static const value_type text_stream_type;
    static const value_type array_type;
    static const value_type dict_type;
    static const value_type any_type;
    static const value_type none_type;
    static const value_type void_type;
    static const value_type fn_type;
    static const value_type unknown_type;

    friend bool operator==(const value_type&, const value_type&) = default;
};

inline const value_type value_type::int_type{"int"};
inline const value_type value_type::bool_type{"bool"};
inline const value_type value_type::float_type{"float"};
inline const value_type value_type::str_type{"str"};
inline const value_type value_type::bytes_type{"bytes"};
inline const value_type value_type::binary_stream_type{"binary_stream"};
inline const value_type value_type::text_stream_type{"text_stream"};
inline const value_type value_type::array_type{"array"};
inline const value_type value_type::dict_type{"dict"};
inline const value_type value_type::any_type{"any"};
inline const value_type value_type::none_type{"none"};
inline const value_type value_type::void_type{"void"};
inline const value_type value_type::fn_type{"fn"};
inline const value_type value_type::unknown_type{"unknown"};

[[nodiscard]] inline std::string_view type_name(const value_type& type)
{
    return type.name;
}

class compile_error final : public std::runtime_error
{
public:
    compile_error(source_pos position, const std::string& message)
        : std::runtime_error(message), position_(position)
    {
    }

    [[nodiscard]] source_pos position() const
    {
        return position_;
    }

private:
    source_pos position_;
};

} // namespace tx
