#pragma once

#include "frontend/ast/ast.hpp"

#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace tx
{

class code_generator
{
public:
    [[nodiscard]] std::string generate(const program& source);

private:
    [[nodiscard]] std::string emit_expression(const expression& item);
    [[nodiscard]] std::string emit_lvalue(const expression& item);
    [[nodiscard]] std::string emit_call(const call_expression& call);
    [[nodiscard]] std::string emit_binary(const binary_operation& operation);
    [[nodiscard]] std::string emit_index(const index_expression& access);
    [[nodiscard]] std::string emit_array(const array_literal& literal);
    [[nodiscard]] std::string ordered_binary(const std::string& left,
                                             const std::string& right,
                                             std::string_view operation,
                                             bool is_function);
    void emit_statement(const statement& item);
    void emit_assignment(const variable_assignment& assignment);
    void emit_statements(const std::vector<stmt_ptr>& statements);
    void emit_for(const for_loop& loop);
    void emit_for_each(const for_each& loop);
    void emit_if(const if_statement& branch);
    void emit_while(const while_statement& loop);
    void emit_struct(const struct_decl& definition);
    void emit_dynamic_fields(const program& source);
    void emit_function(const function_decl& function);
    void write_line(std::string_view text);
    [[nodiscard]] static std::string cpp_type(const value_type& type);
    [[nodiscard]] static std::string cpp_function(const std::string& name);
    [[nodiscard]] static std::string cpp_variable(const std::string& name);
    [[nodiscard]] static std::string cpp_field(const std::string& name);
    [[nodiscard]] static std::string cpp_struct(const std::string& name);
    [[nodiscard]] static std::string function_header(const function_decl& function);

    std::ostringstream output_;
    std::size_t indent_ = 0;
    std::size_t loop_index_ = 0;
    std::size_t expression_index_ = 0;
    std::size_t assignment_index_ = 0;
};

} // namespace tx
