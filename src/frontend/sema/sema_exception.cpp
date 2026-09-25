#include "frontend/sema/sema.hpp"

#include <unordered_set>

namespace tx
{

void semantic_analyzer::check_try(try_statement& guarded)
{
    push_scope();
    check_statements(guarded.body);
    pop_scope();
    std::unordered_set<int> caught;
    for (auto& handler : guarded.handlers)
    {
        const auto found = structs_.find(handler.type.name);
        if (found == structs_.end() ||
            found->second->exception_kind == error_kind::none)
        {
            throw compile_error(handler.position,
                "exception 需要 error.txh 中的 runtime_error、parse_error 或 io_error 类型");
        }
        handler.kind = found->second->exception_kind;
        if (caught.contains(static_cast<int>(error_kind::runtime)))
        {
            throw compile_error(handler.position,
                                "runtime_error 已捕获所有错误，后续 exception 不可达");
        }
        if (!caught.insert(static_cast<int>(handler.kind)).second)
        {
            throw compile_error(handler.position, "重复的 exception 错误类型");
        }
        push_scope();
        declare_symbol(handler.name, {handler.type, false}, handler.position);
        check_statements(handler.body);
        pop_scope();
    }
}

} // namespace tx
