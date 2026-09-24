#pragma once

#include "frontend/ast/ast.hpp"

#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace tx
{

class llvm_code_generator
{
public:
    [[nodiscard]] std::string generate(const program& source);

private:
    struct ir_value
    {
        value_type type;
        std::string text;
    };

    struct variable_slot
    {
        value_type type;
        std::string address;
    };

    [[nodiscard]] static std::string llvm_type(const value_type& type,
                                               source_pos position);
    [[nodiscard]] static bool is_value_handle(const value_type& type);
    [[nodiscard]] static std::string function_name(const std::string& name,
                                                   std::size_t overload);
    [[nodiscard]] std::string temporary();
    [[nodiscard]] std::string label();
    [[nodiscard]] std::string allocate(const value_type& type,
                                       source_pos position);
    [[nodiscard]] std::string global_bytes(std::string_view bytes);
    [[nodiscard]] variable_slot find_variable(const std::string& name,
                                              source_pos position) const;
    [[nodiscard]] ir_value load(const variable_slot& variable);
    void release(const ir_value& value);
    void release_slot(const variable_slot& variable);
    [[nodiscard]] ir_value expression_value(const expression& item);
    [[nodiscard]] ir_value emit_binary(const expression& item,
                                       const binary_operation& operation);
    [[nodiscard]] ir_value emit_call(const expression& item,
                                     const call_expression& call);
    [[nodiscard]] ir_value emit_builtin_call(
        const expression& item, const call_expression& call,
        const std::vector<ir_value>& arguments);
    [[nodiscard]] ir_value emit_constructor_call(
        const expression& item, const call_expression& call,
        const std::vector<ir_value>& arguments);
    [[nodiscard]] ir_value emit_external_call(
        const expression& item, const call_expression& call,
        const function_decl& target, const std::vector<ir_value>& arguments);
    [[nodiscard]] ir_value emit_user_call(
        const expression& item, const call_expression& call,
        const function_decl& target, const std::vector<ir_value>& arguments);
    [[nodiscard]] ir_value emit_cast(const expression& item,
                                     const cast_expression& cast);
    [[nodiscard]] ir_value checked_binary(const std::string& name,
                                          const ir_value& left,
                                          const ir_value& right,
                                          const value_type& result_type,
                                          source_pos position);
    [[nodiscard]] ir_value checked_unary(const std::string& name,
                                         const ir_value& value,
                                         const value_type& result_type,
                                         source_pos position);
    [[nodiscard]] ir_value short_circuit(const binary_operation& operation);
    [[nodiscard]] ir_value box_any(const ir_value& value, source_pos position);
    [[nodiscard]] ir_value from_any(const ir_value& value,
                                    const value_type& target,
                                    source_pos position);
    [[nodiscard]] std::string lvalue_address(const expression& item);
    void emit_statement(const statement& item);
    void emit_declaration(const statement& item,
                          const variable_declaration& declaration);
    void emit_assignment(const statement& item,
                         const variable_assignment& assignment);
    void emit_name_assignment(const statement& item,
                              const variable_assignment& assignment,
                              const name_reference& name);
    void emit_composite_assignment(const statement& item,
                                   const variable_assignment& assignment);
    void emit_return(const statement& item, const return_statement& result);
    void emit_statements(const std::vector<stmt_ptr>& statements);
    void emit_if(const if_statement& branch);
    void emit_while(const while_statement& loop);
    void emit_for(const for_loop& loop);
    void emit_for_each(const for_each& loop);
    void emit_function(const function_decl& function);
    void write_instruction(const std::string& text);
    void start_block(const std::string& name);
    void push_scope();
    void pop_scope();

    std::unordered_map<std::string, std::vector<const function_decl*>> functions_;
    std::unordered_map<std::string, const struct_decl*> structs_;
    std::vector<std::unordered_map<std::string, variable_slot>> scopes_;
    std::ostringstream module_;
    std::ostringstream globals_;
    std::ostringstream allocations_;
    std::ostringstream body_;
    value_type return_type_ = value_type::void_type;
    std::size_t next_value_ = 0;
    std::size_t next_slot_ = 0;
    std::size_t next_label_ = 0;
    std::size_t next_string_ = 0;
    bool terminated_ = false;
};

} // namespace tx
