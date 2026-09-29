#pragma once

#include "stdlib/format_stream.hpp"
#include "stdlib/json.hpp"
#include <variant>

namespace tx_generated
{

class json_parser
{
public:
    explicit json_parser(format_input& input, std::size_t max_depth = 128,
                         bool reject_duplicate_keys = false);
    std::any parse();
    std::any parse_value(std::size_t depth);
    void skip_space();
    void require_end();
    std::string parse_string();
    std::variant<std::int64_t, double> parse_numeric();
    void skip_value(std::size_t depth);

private:
    std::any parse_literal(std::string_view literal, std::any value);
    tx_array parse_array(std::size_t depth);
    tx_dict parse_object(std::size_t depth);
    std::any parse_number();
    std::uint32_t parse_hex_quad();
    void parse_unicode_escape(std::string& output);
    void parse_escape(std::string& output);
    format_input& input_;
    std::size_t max_depth_;
    bool reject_duplicate_keys_ = false;
};

} // namespace tx_generated
