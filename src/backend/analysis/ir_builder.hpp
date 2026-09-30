#pragma once

#include "backend/analysis/program_ir.hpp"

namespace tx
{

class ir_builder
{
public:
    ir_builder(const function_decl& function, const function_lookup& functions, const program& source);
    [[nodiscard]] function_ir build();

private:
    function_ir result_;
    const function_lookup& functions_;
    const program& source_;
    analysis_id current_ = 0;
    std::vector<std::unordered_map<std::string, analysis_id>> scopes_;
    std::vector<analysis_id> exception_targets_;
    std::vector<std::size_t> exception_depths_;

    analysis_id block();
    void edge(analysis_id from, analysis_id to, bool exceptional = false);
    analysis_id append(analysis_instruction instruction, bool may_fail = false);
    void cleanup_scopes(std::size_t depth, bool may_fail);
    void finalize_parameter_cleanups();
    [[nodiscard]] bool inert_destruction(const value_type& type, std::set<std::string>& visiting) const;
    analysis_id variable(std::string_view name) const;
    analysis_id declare(std::string name, const value_type& type,
        const variable_declaration* declaration = nullptr,
        analysis_id parameter = no_analysis_id);
    analysis_id bind(analysis_id variable, analysis_id value);
    analysis_id read(analysis_id variable, const expression* source = nullptr);
    analysis_id value(const expression& item, bool transfer = false);
    analysis_id logical_value(const expression& item, const binary_operation& operation);
    analysis_id call_value(const expression& item, const call_expression& call);
    void lower_arguments(analysis_instruction& instruction, const call_expression& call);
    analysis_id operation_value(const expression& item, const operator_binding& binding,
        std::vector<analysis_id> inputs);
    void statements(const std::vector<stmt_ptr>& body, bool scoped = true);
    void statement_value(const statement& item);
    void declaration_value(const variable_declaration& declaration);
    void unpack_value(const unpack_assignment& unpack);
    void assignment_value(const variable_assignment& assignment);
    void branch_value(const if_statement& branch);
    void while_value(const while_statement& loop);
    void range_value(const for_loop& loop);
    void foreach_value(const for_each& loop);
    void try_value(const try_statement& guarded);
    void condition(analysis_id value, analysis_id yes, analysis_id no);
    const function_decl* target(std::string_view symbol, std::size_t overload) const;
};

} // namespace tx
