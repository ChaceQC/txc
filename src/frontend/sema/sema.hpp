#pragma once

#include "frontend/ast/ast.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace tx
{

struct symbol_info
{
    value_type type;
    bool read_only = false;
};

struct function_signature
{
    std::vector<parameter> parameters;
    value_type result;
    bool accepts_array_value = false;
};

class semantic_analyzer
{
public:
    void analyze(program& source, bool require_main = true);

private:
    struct field_match
    {
        const class_field* field;
        const class_decl* owner;
    };
    void register_structs(program& source);
    void index_classes(program& source);
    void register_classes(program& source);
    void register_class(class_decl& definition, program& source,
                        std::unordered_map<std::string, int>& state);
    void register_class_methods(class_decl& definition, program& source);
    void register_class_method(class_decl& definition, function_decl& method,
                               program& source);
    void register_functions(const program& source, bool require_main);
    void validate_type(const value_type& type, source_pos position) const;
    void check_function(function_decl& function);
    void check_method(function_decl& method);
    void check_statements(std::vector<stmt_ptr>& statements);
    void check_statement(statement& item);
    void check_declaration(statement& item, variable_declaration& declaration);
    void check_assignment(statement& item, variable_assignment& assignment);
    void check_unpack(statement& item, unpack_assignment& assignment);
    void check_for(statement& item, for_loop& loop);
    void check_for_each(statement& item, for_each& loop);
    void check_if(statement& item, if_statement& branch);
    void check_while(statement& item, while_statement& loop);
    void check_try(try_statement& guarded);
    [[nodiscard]] value_type check_lvalue(expression& target);
    [[nodiscard]] value_type check_expression(expression& item);
    [[nodiscard]] value_type check_literal(expression& item);
    [[nodiscard]] value_type check_unary(expression& item, unary_operation& operation);
    [[nodiscard]] value_type check_binary(expression& item, binary_operation& operation);
    void validate_operator_method(const function_decl& method) const;
    [[nodiscard]] value_type bind_operator(
        token_kind kind, const value_type& receiver,
        const std::optional<value_type>& argument,
        const expression& receiver_expression, source_pos position,
        std::optional<operator_binding>& binding) const;
    [[nodiscard]] value_type check_call(expression& item, call_expression& call);
    [[nodiscard]] value_type check_builtin(expression& item, call_expression& call);
    [[nodiscard]] value_type check_constructor(expression& item, call_expression& call);
    [[nodiscard]] value_type check_class_constructor(expression& item, call_expression& call);
    [[nodiscard]] value_type check_method_call(expression& item, call_expression& call);
    [[nodiscard]] value_type check_vector_call(expression& item, call_expression& call,
                                              const value_type& type);
    [[nodiscard]] value_type check_container_call(expression& item, call_expression& call,
                                                 const value_type& type);
    [[nodiscard]] std::vector<value_type> check_call_arguments(call_expression& call);
    [[nodiscard]] bool matches_signature(
        const function_signature& signature, const call_expression& call,
        const std::vector<value_type>& types, bool allow_upcast) const;
    [[nodiscard]] value_type check_index(expression& item, index_expression& access);
    [[nodiscard]] value_type check_member(expression& item, member_expression& access);
    [[nodiscard]] value_type check_cast(expression& item, cast_expression& cast);
    [[nodiscard]] symbol_info* find_symbol(const std::string& name);
    void declare_symbol(const std::string& name, symbol_info info, source_pos position);
    void require_type(const value_type& actual, const value_type& expected,
                      source_pos position, const std::string& context);
    [[nodiscard]] bool is_assignable(const value_type& actual,
                                     const value_type& expected) const;
    [[nodiscard]] std::size_t class_distance(const value_type& actual,
                                             const value_type& expected) const;
    [[nodiscard]] std::vector<const function_decl*> collect_methods(
        const class_decl& type, std::string_view name) const;
    [[nodiscard]] std::vector<field_match> collect_fields(
        const class_decl& type, std::string_view name) const;
    [[nodiscard]] const class_decl* first_class_base(
        const class_decl& type) const;
    [[nodiscard]] static bool same_overload_key(
        const function_decl& left, const function_decl& right);
    [[nodiscard]] const function_decl* select_method(
        const std::vector<const function_decl*>& candidates,
        const call_expression& call, const std::vector<value_type>& types,
        source_pos position, std::string_view description) const;
    [[nodiscard]] std::size_t method_conversion_cost(
        const function_decl& method, const call_expression& call,
        const std::vector<value_type>& types) const;
    void check_access(member_access access, const class_decl& owner,
                      source_pos position, std::string_view member) const;
    void push_scope();
    void pop_scope();

    std::unordered_map<std::string, const struct_decl*> structs_;
    std::unordered_map<std::string, class_decl*> classes_;
    std::unordered_map<std::string, std::vector<function_signature>> functions_;
    std::vector<std::unordered_map<std::string, symbol_info>> scopes_;
    value_type current_return_type_ = value_type::unknown_type;
    const class_decl* current_class_ = nullptr;
};

} // namespace tx
