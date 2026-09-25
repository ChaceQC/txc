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

    [[nodiscard]] bool is_vector() const noexcept
    {
        return parameters.size() == 1;
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
    static const value_type array_type;
    static const value_type dict_type;
    static const value_type any_type;
    static const value_type none_type;
    static const value_type void_type;
    static const value_type unknown_type;

    friend bool operator==(const value_type&, const value_type&) = default;
};

inline const value_type value_type::int_type{"int"};
inline const value_type value_type::bool_type{"bool"};
inline const value_type value_type::float_type{"float"};
inline const value_type value_type::str_type{"str"};
inline const value_type value_type::array_type{"array"};
inline const value_type value_type::dict_type{"dict"};
inline const value_type value_type::any_type{"any"};
inline const value_type value_type::none_type{"none"};
inline const value_type value_type::void_type{"void"};
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
