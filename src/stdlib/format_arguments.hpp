#pragma once

#include "backend/cpp/format_arguments_abi.hpp"
#include "stdlib/stdlib.hpp"

namespace tx_generated
{

struct legacy_format_arguments
{
    const tx_array& positional;
    const tx_dict& keywords;

    std::size_t size() const noexcept
    {
        return positional.size();
    }
    const std::any& operator[](std::size_t index) const
    {
        return positional[index];
    }
    const std::any* find(std::string_view name) const
    {
        return keywords.find_value(name);
    }
};

struct direct_format_arguments
{
    std::span<const format_argument> positional;
    std::span<const format_argument> keywords;

    std::size_t size() const noexcept
    {
        return positional.size();
    }
    const format_argument& operator[](std::size_t index) const
    {
        return positional[index];
    }
    const format_argument* find(std::string_view name) const
    {
        for (const auto& argument : keywords)
        {
            if (name == argument.name)
            {
                return &argument;
            }
        }
        return nullptr;
    }
};

inline void append_format_value(std::string& output, const format_argument& argument,
    const tx::format_spec& spec, char conversion)
{
    argument.append(output, argument.bits, spec, conversion);
}

} // namespace tx_generated
