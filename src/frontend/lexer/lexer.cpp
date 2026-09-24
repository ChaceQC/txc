#include "frontend/lexer/lexer.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

namespace tx
{
namespace
{

bool is_digit(char value)
{
    return value >= '0' && value <= '9';
}

bool is_letter(char value)
{
    return (value >= 'a' && value <= 'z') ||
           (value >= 'A' && value <= 'Z') || value == '_';
}

std::optional<token_kind> keyword_kind(std::string_view text)
{
    static const std::unordered_map<std::string_view, token_kind> keywords = {
        {"def", token_kind::keyword_def},
        {"auto", token_kind::keyword_auto},
        {"for", token_kind::keyword_for},
        {"in", token_kind::keyword_in},
        {"return", token_kind::keyword_return},
        {"if", token_kind::keyword_if},
        {"else", token_kind::keyword_else},
        {"while", token_kind::keyword_while},
        {"true", token_kind::keyword_true},
        {"false", token_kind::keyword_false},
        {"int", token_kind::keyword_int},
        {"bool", token_kind::keyword_bool},
        {"float", token_kind::keyword_float},
        {"str", token_kind::keyword_str},
        {"array", token_kind::keyword_array},
        {"struct", token_kind::keyword_struct},
        {"as", token_kind::keyword_as},
        {"none", token_kind::keyword_none},
        {"import", token_kind::keyword_import}};

    if (const auto found = keywords.find(text); found != keywords.end())
    {
        return found->second;
    }
    return std::nullopt;
}

} // namespace

lexer::lexer(std::string_view source, std::string file)
    : source_(source), file_(std::move(file))
{
}

bool lexer::at_end() const
{
    return index_ >= source_.size();
}

bool lexer::starts_with(std::string_view text) const
{
    return source_.substr(index_, text.size()) == text;
}

char lexer::current() const
{
    return source_[index_];
}

source_pos lexer::position() const
{
    return {line_, column_, file_};
}

void lexer::advance()
{
    if (source_[index_++] == '\n')
    {
        ++line_;
        column_ = 1;
    }
    else
    {
        ++column_;
    }
}

void lexer::take_symbol(token_kind kind, std::size_t length)
{
    const auto start = index_;
    const auto start_pos = position();
    for (std::size_t count = 0; count < length; ++count)
    {
        advance();
    }
    tokens_.push_back({kind, std::string(source_.substr(start, length)), start_pos});
}

void lexer::scan_identifier()
{
    const auto start = index_;
    const auto start_pos = position();
    while (!at_end() && (is_letter(current()) || is_digit(current())))
    {
        advance();
    }
    const auto text = source_.substr(start, index_ - start);
    tokens_.push_back({keyword_kind(text).value_or(token_kind::identifier),
                       std::string(text), start_pos});
}

void lexer::scan_number()
{
    const auto start = index_;
    const auto start_pos = position();
    while (!at_end() && is_digit(current()))
    {
        advance();
    }
    bool is_float = false;
    if (!at_end() && current() == '.' &&
        index_ + 1 < source_.size() && is_digit(source_[index_ + 1]))
    {
        is_float = true;
        advance();
        while (!at_end() && is_digit(current()))
        {
            advance();
        }
    }
    if (!at_end() && (current() == 'e' || current() == 'E'))
    {
        is_float = true;
        advance();
        if (!at_end() && (current() == '+' || current() == '-'))
        {
            advance();
        }
        if (at_end() || !is_digit(current()))
        {
            throw compile_error(position(), "浮点指数缺少数字");
        }
        while (!at_end() && is_digit(current()))
        {
            advance();
        }
    }
    tokens_.push_back({is_float ? token_kind::floating : token_kind::integer,
                       std::string(source_.substr(start, index_ - start)), start_pos});
}

void lexer::scan_string()
{
    const auto start = index_;
    const auto start_pos = position();
    advance();
    while (!at_end() && current() != '"')
    {
        if (current() == '\n')
        {
            throw compile_error(start_pos, "字符串不能跨行");
        }
        if (current() == '\\')
        {
            advance();
            if (at_end() || (current() != '"' && current() != '\\' &&
                             current() != 'n' && current() != 'r' &&
                             current() != 't'))
            {
                throw compile_error(position(), "不支持的字符串转义");
            }
        }
        advance();
    }
    if (at_end())
    {
        throw compile_error(start_pos, "字符串缺少结束引号");
    }
    advance();
    tokens_.push_back({token_kind::string_literal,
                       std::string(source_.substr(start, index_ - start)), start_pos});
}

void lexer::scan_symbol()
{
    static constexpr struct
    {
        std::string_view text;
        token_kind kind;
    } symbols[] = {
        {"..=", token_kind::range_inclusive},
        {"->", token_kind::arrow},
        {"+=", token_kind::plus_equal},
        {"==", token_kind::equal_equal},
        {"!=", token_kind::bang_equal},
        {"<=", token_kind::less_equal},
        {">=", token_kind::greater_equal},
        {"&&", token_kind::and_and},
        {"||", token_kind::or_or},
        {"(", token_kind::left_paren},
        {")", token_kind::right_paren},
        {"[", token_kind::left_bracket},
        {"]", token_kind::right_bracket},
        {"{", token_kind::left_brace},
        {"}", token_kind::right_brace},
        {",", token_kind::comma},
        {":", token_kind::colon},
        {".", token_kind::dot},
        {"+", token_kind::plus},
        {"-", token_kind::minus},
        {"*", token_kind::star},
        {"/", token_kind::slash},
        {"=", token_kind::equal},
        {"!", token_kind::bang},
        {"<", token_kind::less},
        {">", token_kind::greater}};

    for (const auto& symbol : symbols)
    {
        if (!starts_with(symbol.text))
        {
            continue;
        }
        if (symbol.kind == token_kind::right_paren && paren_depth_ == 0)
        {
            throw compile_error(position(), "多余的右括号");
        }
        if (symbol.kind == token_kind::right_bracket && bracket_depth_ == 0)
        {
            throw compile_error(position(), "多余的右方括号");
        }
        if (symbol.kind == token_kind::left_paren)
        {
            ++paren_depth_;
        }
        if (symbol.kind == token_kind::right_paren)
        {
            --paren_depth_;
        }
        if (symbol.kind == token_kind::left_bracket)
        {
            ++bracket_depth_;
        }
        if (symbol.kind == token_kind::right_bracket)
        {
            --bracket_depth_;
        }
        take_symbol(symbol.kind, symbol.text.size());
        return;
    }
    throw compile_error(position(), "无法识别的字符");
}

std::vector<token> lexer::scan()
{
    if (source_.substr(0, 3) == "\xEF\xBB\xBF")
    {
        index_ = 3;
    }
    while (!at_end())
    {
        if (current() == ' ' || current() == '\t' || current() == '\r')
        {
            advance();
        }
        else if (current() == '\n')
        {
            if (paren_depth_ == 0 && bracket_depth_ == 0)
            {
                tokens_.push_back({token_kind::newline, "\n", position()});
            }
            advance();
        }
        else if (current() == '#' || starts_with("//"))
        {
            while (!at_end() && current() != '\n')
            {
                advance();
            }
        }
        else if (is_letter(current()))
        {
            scan_identifier();
        }
        else if (is_digit(current()))
        {
            scan_number();
        }
        else if (current() == '"')
        {
            scan_string();
        }
        else
        {
            scan_symbol();
        }
    }
    if (paren_depth_ != 0)
    {
        throw compile_error(position(), "缺少右括号");
    }
    if (bracket_depth_ != 0)
    {
        throw compile_error(position(), "缺少右方括号");
    }
    tokens_.push_back({token_kind::end_of_file, "", position()});
    return std::move(tokens_);
}

} // namespace tx
