#pragma once

#include "frontend/lexer/token.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace tx
{

class lexer
{
public:
    explicit lexer(std::string_view source, std::string file = {});
    [[nodiscard]] std::vector<token> scan();

private:
    [[nodiscard]] bool at_end() const;
    [[nodiscard]] bool starts_with(std::string_view text) const;
    [[nodiscard]] char current() const;
    [[nodiscard]] source_pos position() const;
    void advance();
    void take_symbol(token_kind kind, std::size_t length);
    void scan_identifier();
    void scan_number();
    void scan_string();
    void scan_symbol();

    std::string_view source_;
    std::string file_;
    std::vector<token> tokens_;
    std::size_t index_ = 0;
    std::size_t line_ = 1;
    std::size_t column_ = 1;
    std::size_t paren_depth_ = 0;
    std::size_t bracket_depth_ = 0;
};

} // namespace tx
