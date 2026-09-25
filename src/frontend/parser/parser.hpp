#pragma once

#include "frontend/ast/ast.hpp"

#include <vector>

namespace tx
{

class parser
{
public:
    explicit parser(std::vector<token> tokens, bool interface_mode = false);
    [[nodiscard]] program parse_program();

private:
    [[nodiscard]] const token& current() const;
    [[nodiscard]] const token& previous() const;
    [[nodiscard]] const token& advance();
    [[nodiscard]] bool check(token_kind kind) const;
    [[nodiscard]] bool match(token_kind kind);
    [[nodiscard]] const token& consume(token_kind kind, const char* message);
    void skip_newlines();
    [[nodiscard]] value_type parse_type();
    [[nodiscard]] import_decl parse_import();
    [[nodiscard]] struct_decl parse_struct();
    [[nodiscard]] class_decl parse_class(bool is_abstract = false,
                                         bool is_interface = false);
    [[nodiscard]] function_decl parse_function(bool declaration_only = false);
    [[nodiscard]] std::vector<stmt_ptr> parse_block();
    [[nodiscard]] stmt_ptr parse_statement();
    [[nodiscard]] bool looks_like_declaration() const;
    [[nodiscard]] stmt_ptr parse_for(source_pos position);
    [[nodiscard]] stmt_ptr parse_if(source_pos position);
    [[nodiscard]] stmt_ptr parse_while(source_pos position);
    [[nodiscard]] expr_ptr parse_expression(int min_precedence = 0);
    [[nodiscard]] expr_ptr parse_unary();
    [[nodiscard]] expr_ptr parse_postfix();
    [[nodiscard]] expr_ptr parse_atom();
    [[nodiscard]] std::vector<call_argument> parse_arguments();

    std::vector<token> tokens_;
    std::size_t index_ = 0;
    bool interface_mode_ = false;
};

} // namespace tx
