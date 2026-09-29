#pragma once

#include "common/common.hpp"
#include "common/call_properties.hpp"
#include "common/error_kind.hpp"
#include "frontend/lexer/token.hpp"

#include <memory>
#include <cstdint>
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
    std::string function_symbol = {};
    bool ambiguous_function = false;
    bool function_value = false;
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

struct operator_binding
{
    std::string symbol;
    std::size_t overload_index = 0;
    std::optional<std::size_t> virtual_slot;
};

struct unary_operation
{
    token_kind operation;
    expr_ptr operand;
    bool is_min_int_literal = false;
    std::optional<operator_binding> binding;
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
    std::optional<operator_binding> binding;
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
    std::optional<value_type> container_type = std::nullopt;
    bool indirect = false;
    std::optional<value_type> expected_result = std::nullopt;
    std::optional<value_type> explicit_type = std::nullopt;
    call_properties properties = {};
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
    std::uint64_t ownership_id = 0;
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
    std::optional<operator_binding> binding;
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

struct exception_clause
{
    value_type type;
    std::string name;
    std::vector<stmt_ptr> body;
    source_pos position;
    error_kind kind = error_kind::none;
};

struct try_statement
{
    std::vector<stmt_ptr> body;
    std::vector<exception_clause> handlers;
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
    try_statement,
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
    std::shared_ptr<expression> default_value;
};

[[nodiscard]] inline bool parameter_is_nullable(const parameter& value)
{
    return value.default_value &&
        std::holds_alternative<none_literal>(value.default_value->data);
}

struct struct_field
{
    std::string name;
    value_type type;
    source_pos position;
    struct serde_field_metadata
    {
        std::int64_t number = 0;
        bool unknown_capture = false;
        std::optional<std::string> default_literal;
        friend bool operator==(const serde_field_metadata&,
                               const serde_field_metadata&) = default;
    };
    std::optional<serde_field_metadata> serde;
};

enum class serde_unknown_policy { reject, ignore, preserve };

struct serde_struct_metadata
{
    std::int64_t version = 0;
    serde_unknown_policy unknown = serde_unknown_policy::reject;
    std::vector<std::int64_t> reserved;
    friend bool operator==(const serde_struct_metadata&,
                           const serde_struct_metadata&) = default;
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
    std::optional<token_kind> operator_kind;
    std::vector<std::string> type_parameters;
    bool is_async = false;
};

struct struct_decl
{
    std::string name;
    std::vector<struct_field> fields;
    source_pos position;
    std::vector<function_decl> methods;
    error_kind exception_kind = error_kind::none;
    std::optional<serde_struct_metadata> serde;
    std::string source_name;
    bool native_layout = false;
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
    bool native_layout = false;
};

[[nodiscard]] inline std::string class_method_symbol(
    std::string_view owner, std::string_view method)
{
    return "$class$" + std::to_string(owner.size()) + "$" +
           std::string(owner) + "$" + std::string(method);
}

[[nodiscard]] inline std::string operator_method_name(token_kind kind)
{
    switch (kind)
    {
    case token_kind::plus: return "$operator_plus";
    case token_kind::minus: return "$operator_minus";
    case token_kind::star: return "$operator_multiply";
    case token_kind::slash: return "$operator_divide";
    case token_kind::ampersand: return "$operator_bit_and";
    case token_kind::caret: return "$operator_bit_xor";
    case token_kind::pipe: return "$operator_bit_or";
    case token_kind::equal_equal: return "$operator_equal";
    case token_kind::bang_equal: return "$operator_not_equal";
    case token_kind::less: return "$operator_less";
    case token_kind::less_equal: return "$operator_less_equal";
    case token_kind::greater: return "$operator_greater";
    case token_kind::greater_equal: return "$operator_greater_equal";
    case token_kind::bang: return "$operator_not";
    default: return {};
    }
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
    std::string symbol_prefix;
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
