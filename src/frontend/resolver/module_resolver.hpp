#pragma once

#include "frontend/ast/ast.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace tx
{

class module_resolver
{
public:
    void resolve(program& source);

private:
    struct export_entry
    {
        std::string internal_name;
        bool is_struct = false;
        source_pos position;
    };

    void build_index(const program& source);
    [[nodiscard]] std::string module_of(const source_pos& position) const;
    [[nodiscard]] const export_entry* find_export(
        const std::string& module_key, const std::string& name) const;
    [[nodiscard]] const export_entry* find_symbol(
        const std::string& module_key, const std::string& name,
        source_pos position, bool type_only) const;
    [[nodiscard]] value_type resolve_type(
        const std::string& module_key, const value_type& type,
        source_pos position) const;
    [[nodiscard]] std::string resolve_call_name(
        const std::string& module_key, const std::string& name,
        source_pos position) const;
    void check_local_name(const std::string& module_key,
                          const std::string& name, source_pos position) const;
    void resolve_statements(std::vector<stmt_ptr>& statements,
                            const std::string& module_key);
    void resolve_statement(statement& item, const std::string& module_key);
    void resolve_expression(expression& item, const std::string& module_key);

    const program* source_ = nullptr;
    std::unordered_map<std::string, const module_scope*> scopes_;
    std::unordered_map<std::string, std::string> prefixes_;
    std::unordered_map<std::string,
        std::unordered_map<std::string, export_entry>> exports_;
};

} // namespace tx
