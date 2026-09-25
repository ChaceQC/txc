#include "frontend/parser/parser.hpp"

#include <memory>
#include <utility>

namespace tx
{

stmt_ptr parser::parse_try(source_pos position)
{
    try_statement result;
    result.body = parse_block();
    while (true)
    {
        const auto saved_index = index_;
        skip_newlines();
        if (!match(token_kind::keyword_exception))
        {
            index_ = saved_index;
            break;
        }
        const auto handler_position = previous().position;
        auto type = parse_type();
        (void)consume(token_kind::keyword_as, "exception 类型后需要 as");
        auto name = consume(token_kind::identifier, "as 后需要错误变量名").text;
        auto body = parse_block();
        result.handlers.push_back({std::move(type), std::move(name),
                                   std::move(body), handler_position});
    }
    if (result.handlers.empty())
    {
        throw compile_error(position, "try 后至少需要一个 exception 类型 as 变量分支");
    }
    return std::make_unique<statement>(position, std::move(result));
}

} // namespace tx
