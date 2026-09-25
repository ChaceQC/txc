#pragma once

#include "common/common.hpp"
#include "frontend/lexer/token.hpp"

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace tx
{

struct expression;
struct statement;

using expr_ptr = std::unique_ptr<expression>;
using stmt_ptr = std::unique_ptr<statement>;

struct integer_literal
{
    std::string digits;
};

struct boolean_literal
{
    bool value;
};

struct floating_literal
{
    std::string digits;
};

struct string_literal
{
    std::string text;
};

struct none_literal
{
};

struct array_literal
{
    std::vector<expr_ptr> elements;
};

struct dictionary_entry
{
    expr_ptr key;
    expr_ptr value;
};

struct dictionary_literal
{
    std::vector<dictionary_entry> entries;
};

struct name_reference
{
    std::string name;
};

struct index_expression
{
    expr_ptr object;
    expr_ptr index;
};

struct member_expression
{
    expr_ptr object;
    std::string field;
};

struct cast_expression
{
    expr_ptr value;
    value_type target;
};

struct unary_operation
{
    token_kind operation;
    expr_ptr operand;
    bool is_min_int_literal = false;
};

struct update_expression
{
    token_kind operation;
    expr_ptr target;
};

struct binary_operation
{
    token_kind operation;
    expr_ptr left;
    expr_ptr right;
};

enum class argument_kind { positional, keyword, spread_array, spread_dict };

struct call_argument
{
    argument_kind kind;
    std::string name;
    expr_ptr value;
    source_pos position;
};

struct call_expression
{
    std::string name;
    std::vector<call_argument> arguments;
    bool is_constructor = false;
    std::string source_name;
    std::optional<std::size_t> overload_index = std::nullopt;
};

using expression_data = std::variant<
    integer_literal,
    boolean_literal,
    floating_literal,
    string_literal,
    none_literal,
    array_literal,
    dictionary_literal,
    name_reference,
    index_expression,
    member_expression,
    cast_expression,
    unary_operation,
    update_expression,
    binary_operation,
    call_expression>;

struct expression
{
    expression(source_pos position, expression_data data);
    ~expression();

    source_pos position;
    value_type type = value_type::unknown_type;
    expression_data data;
};

struct variable_declaration
{
    std::string name;
    std::optional<value_type> declared_type;
    expr_ptr initializer;
    expr_ptr array_length;
};

struct variable_assignment
{
    expr_ptr target;
    token_kind operation;
    expr_ptr value;
};

struct unpack_assignment
{
    std::vector<std::string> names;
    std::vector<bool> declares;
    expr_ptr value;
};

struct for_loop
{
    std::string name;
    expr_ptr first;
    expr_ptr last;
    std::vector<stmt_ptr> body;
};

struct for_each
{
    std::string name;
    expr_ptr values;
    std::vector<stmt_ptr> body;
};

struct if_statement
{
    expr_ptr condition;
    std::vector<stmt_ptr> then_body;
    std::vector<stmt_ptr> else_body;
    bool has_else = false;
};

struct while_statement
{
    expr_ptr condition;
    std::vector<stmt_ptr> body;
};

struct return_statement
{
    expr_ptr value;
};

struct expression_statement
{
    expr_ptr value;
};

using statement_data = std::variant<
    variable_declaration,
    variable_assignment,
    unpack_assignment,
    for_loop,
    for_each,
    if_statement,
    while_statement,
    return_statement,
    expression_statement>;

struct statement
{
    statement(source_pos position, statement_data data);
    ~statement();

    source_pos position;
    statement_data data;
};

enum class parameter_kind { ordinary, variadic_array, variadic_dict };

struct parameter
{
    std::string name;
    value_type type;
    source_pos position;
    parameter_kind kind = parameter_kind::ordinary;
};

struct struct_field
{
    std::string name;
    value_type type;
    source_pos position;
};

struct struct_decl
{
    std::string name;
    std::vector<struct_field> fields;
    source_pos position;
};

struct function_decl
{
    std::string name;
    std::vector<parameter> parameters;
    value_type return_type;
    std::vector<stmt_ptr> body;
    source_pos position;
    bool external = false;
    std::string source_name;
    std::string external_name;
};

struct import_decl
{
    std::string path;
    std::string alias;
    source_pos position;
};

struct module_import_binding
{
    std::string alias;
    std::string target;
    source_pos position;
};

struct module_scope
{
    std::string key;
    std::vector<module_import_binding> imports;
};

struct program
{
    std::vector<import_decl> imports;
    std::vector<struct_decl> structs;
    std::vector<function_decl> functions;
    std::vector<module_scope> modules;
    std::unordered_map<std::string, std::string> file_modules;
    std::string root_module;
};

} // namespace tx
