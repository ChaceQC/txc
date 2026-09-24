#include "frontend/ast/ast.hpp"

#include <utility>

namespace tx
{

expression::expression(source_pos position, expression_data data)
    : position(position), data(std::move(data))
{
}

expression::~expression() = default;

statement::statement(source_pos position, statement_data data)
    : position(position), data(std::move(data))
{
}

statement::~statement() = default;

} // namespace tx
