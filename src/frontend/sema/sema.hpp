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
    std::vector<value_type> parameters;
    value_type result;
};

class semantic_analyzer
{
public:
    void analyze(program& source, bool require_main = true);

private:
    void register_structs(const program& source);
    void register_functions(const program& source, bool require_main);
    void validate_type(const value_type& type, source_pos position) const;
    void check_function(function_decl& function);
    void check_statements(std::vector<stmt_ptr>& statements);
    void check_statement(statement& item);
    void check_declaration(statement& item, variable_declaration& declaration);
    void check_assignment(statement& item, variable_assignment& assignment);
    void check_for(statement& item, for_loop& loop);
    void check_for_each(statement& item, for_each& loop);
    void check_if(statement& item, if_statement& branch);
    void check_while(statement& item, while_statement& loop);
    [[nodiscard]] value_type check_lvalue(expression& target);
    [[nodiscard]] value_type check_expression(expression& item);
    [[nodiscard]] value_type check_literal(expression& item);
    [[nodiscard]] value_type check_unary(expression& item, unary_operation& operation);
    [[nodiscard]] value_type check_binary(expression& item, binary_operation& operation);
    [[nodiscard]] value_type check_call(expression& item, call_expression& call);
    [[nodiscard]] value_type check_builtin(expression& item, call_expression& call);
    [[nodiscard]] value_type check_constructor(expression& item, call_expression& call);
    [[nodiscard]] value_type check_index(expression& item, index_expression& access);
    [[nodiscard]] value_type check_member(expression& item, member_expression& access);
    [[nodiscard]] value_type check_cast(expression& item, cast_expression& cast);
    [[nodiscard]] symbol_info* find_symbol(const std::string& name);
    void declare_symbol(const std::string& name, symbol_info info, source_pos position);
    void require_type(const value_type& actual, const value_type& expected,
                      source_pos position, const std::string& context);
    void push_scope();
    void pop_scope();

    std::unordered_map<std::string, const struct_decl*> structs_;
    std::unordered_map<std::string, std::vector<function_signature>> functions_;
    std::vector<std::unordered_map<std::string, symbol_info>> scopes_;
    value_type current_return_type_ = value_type::unknown_type;
};

} // namespace tx
