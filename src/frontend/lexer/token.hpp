#pragma once

#include "common/common.hpp"

#include <string>

namespace tx
{

enum class token_kind
{
    end_of_file,
    newline,
    identifier,
    integer,
    floating,
    string_literal,
    keyword_def,
    keyword_auto,
    keyword_for,
    keyword_in,
    keyword_return,
    keyword_if,
    keyword_else,
    keyword_while,
    keyword_try,
    keyword_exception,
    keyword_true,
    keyword_false,
    keyword_int,
    keyword_bool,
    keyword_float,
    keyword_str,
    keyword_array,
    keyword_dict,
    keyword_struct,
    keyword_class,
    keyword_interface,
    keyword_abstract,
    keyword_public,
    keyword_protected,
    keyword_private,
    keyword_virtual,
    keyword_override,
    keyword_as,
    keyword_none,
    keyword_import,
    left_paren,
    right_paren,
    left_bracket,
    right_bracket,
    left_brace,
    right_brace,
    comma,
    colon,
    dot,
    arrow,
    range_inclusive,
    plus,
    plus_plus,
    minus,
    minus_minus,
    star,
    double_star,
    slash,
    plus_equal,
    minus_equal,
    equal,
    equal_equal,
    bang,
    bang_equal,
    less,
    less_equal,
    greater,
    greater_equal,
    and_and,
    or_or
};

struct token
{
    token_kind kind;
    std::string text;
    source_pos position;
};

} // namespace tx
