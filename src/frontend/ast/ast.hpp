#pragma once

#include "common/common.hpp"
#include "frontend/lexer/token.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
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
    std::optional<std::size_t> field_slot;
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
    expr_ptr receiver;
    bool virtual_dispatch = false;
    std::size_t virtual_slot = 0;
    std::string constructor_init_symbol;
    std::size_t constructor_init_index = 0;
    bool is_super_view = false;
    std::string super_type;
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

enum class member_access { private_access, protected_access, public_access };

struct class_field
{
    std::string name;
    value_type type;
    source_pos position;
    member_access access = member_access::private_access;
    std::size_t slot = 0;
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
    std::string owner_class;
    member_access access = member_access::public_access;
    bool is_virtual = false;
    bool is_override = false;
    bool is_abstract = false;
    std::vector<std::size_t> virtual_slots;
    std::size_t overload_index = 0;
};

struct virtual_target
{
    std::string symbol;
    std::size_t overload_index = 0;
    std::string owner_class;
};

struct class_decl
{
    std::string name;
    std::vector<std::string> bases;
    std::vector<class_field> fields;
    std::vector<function_decl> methods;
    source_pos position;
    std::vector<virtual_target> virtual_targets;
    std::vector<std::size_t> known_virtual_slots;
    bool is_interface = false;
    bool is_abstract = false;
    std::string source_name;
};

[[nodiscard]] inline std::string class_method_symbol(
    std::string_view owner, std::string_view method)
{
    return "$class$" + std::to_string(owner.size()) + "$" +
           std::string(owner) + "$" + std::string(method);
}

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
    std::vector<class_decl> classes;
    std::vector<function_decl> functions;
    std::vector<module_scope> modules;
    std::unordered_map<std::string, std::string> file_modules;
    std::string root_module;
    std::size_t field_slot_count = 0;
    std::size_t virtual_slot_count = 0;
};

} // namespace tx
