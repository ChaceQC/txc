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

    [[nodiscard]] bool is_option() const noexcept
    {
        return name.starts_with("option<") && parameters.size() == 1;
    }

    [[nodiscard]] bool is_result() const noexcept
    {
        return name.starts_with("result<") && parameters.size() == 1;
    }

    [[nodiscard]] bool is_iterator() const noexcept
    {
        return name.starts_with("iterator<") && parameters.size() == 1;
    }

    [[nodiscard]] bool is_sum_type() const noexcept
    {
        return is_option() || is_result();
    }

    [[nodiscard]] bool is_direct_vector() const noexcept
    {
        return is_vector() &&
            (parameters.front() == int_type || parameters.front() == float_type ||
             parameters.front() == bool_type || parameters.front() == str_type ||
             parameters.front() == bytes_type);
    }

    [[nodiscard]] std::string container_name() const
    {
        return name.substr(0, name.find('<'));
    }

    [[nodiscard]] bool is_map() const noexcept
    {
        return (name.starts_with("map<") || name.starts_with("ordered_map<")) &&
            parameters.size() == 2;
    }

    [[nodiscard]] bool is_entry() const noexcept
    {
        return name.starts_with("entry<") && parameters.size() == 2;
    }

    [[nodiscard]] bool is_priority_entry() const noexcept
    {
        return name.starts_with("priority_entry<") && parameters.size() == 1;
    }

    [[nodiscard]] bool is_deque() const noexcept
    {
        return name.starts_with("deque<") && parameters.size() == 1;
    }

    [[nodiscard]] bool is_typed_container() const noexcept
    {
        return is_map() || (parameters.size() == 1 &&
            (name.starts_with("set<") || name.starts_with("ordered_set<") ||
             name.starts_with("heap<") ||
             name.starts_with("queue<") || name.starts_with("deque<")));
    }

    [[nodiscard]] static bool is_container_name(std::string_view name)
    {
        return name == "vector" || name == "map" || name == "set" ||
               name == "ordered_map" || name == "ordered_set" ||
               name == "entry" || name == "priority_entry" ||
               name == "heap" || name == "queue" || name == "deque" ||
               name == "option" || name == "result" || name == "iterator";
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
    static const value_type cancel_source_type;
    static const value_type cancel_token_type;
    static const value_type encoding_decoder_type;
    static const value_type encoding_encoder_type;
    static const value_type regex_pattern_type;
    static const value_type fs_watcher_type;
    static const value_type process_child_type;
    static const value_type process_pipe_type;
    static const value_type json_reader_type;
    static const value_type json_writer_type;
    static const value_type cbor_reader_type;
    static const value_type cbor_writer_type;
    static const value_type csv_reader_type;
    static const value_type csv_writer_type;
    static const value_type xml_reader_type;
    static const value_type xml_writer_type;
    static const value_type xml_document_type;
    static const value_type xml_node_type;
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
inline const value_type value_type::cancel_source_type{"cancel_source"};
inline const value_type value_type::cancel_token_type{"cancel_token"};
inline const value_type value_type::encoding_decoder_type{"encoding_decoder"};
inline const value_type value_type::encoding_encoder_type{"encoding_encoder"};
inline const value_type value_type::regex_pattern_type{"regex_pattern"};
inline const value_type value_type::fs_watcher_type{"fs_watcher"};
inline const value_type value_type::process_child_type{"process_child"};
inline const value_type value_type::process_pipe_type{"process_pipe"};
inline const value_type value_type::json_reader_type{"json_reader"};
inline const value_type value_type::json_writer_type{"json_writer"};
inline const value_type value_type::cbor_reader_type{"cbor_reader"};
inline const value_type value_type::cbor_writer_type{"cbor_writer"};
inline const value_type value_type::csv_reader_type{"csv_reader"};
inline const value_type value_type::csv_writer_type{"csv_writer"};
inline const value_type value_type::xml_reader_type{"xml_reader"};
inline const value_type value_type::xml_writer_type{"xml_writer"};
inline const value_type value_type::xml_document_type{"xml_document"};
inline const value_type value_type::xml_node_type{"xml_node"};
inline const value_type value_type::array_type{"array"};
inline const value_type value_type::dict_type{"dict"};
inline const value_type value_type::any_type{"any"};
inline const value_type value_type::none_type{"none"};
inline const value_type value_type::void_type{"void"};
inline const value_type value_type::fn_type{"fn"};
inline const value_type value_type::unknown_type{"unknown"};

[[nodiscard]] inline std::string_view type_name(const value_type& type)
{
    const std::string_view name = type.name;
    if (name.starts_with("s_"))
    {
        const auto separator = name.find('_', 2);
        if (separator != std::string_view::npos && separator > 2 &&
            (separator - 2) % 2 == 0 && separator + 1 < name.size())
        {
            bool hex_prefix = true;
            for (std::size_t index = 2; index < separator; ++index)
            {
                const char digit = name[index];
                if (!((digit >= '0' && digit <= '9') ||
                      (digit >= 'a' && digit <= 'f')))
                {
                    hex_prefix = false;
                    break;
                }
            }
            if (hex_prefix)
            {
                return name.substr(separator + 1);
            }
        }
    }
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
