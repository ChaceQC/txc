#include "backend/llvm/codegen.hpp"

namespace tx
{

bool llvm_code_generator::contains_try(const std::vector<stmt_ptr>& body)
{
    for (const auto& item : body)
    {
        if (std::holds_alternative<try_statement>(item->data))
        {
            return true;
        }
        if (const auto* branch = std::get_if<if_statement>(&item->data);
            branch && (contains_try(branch->then_body) || contains_try(branch->else_body)))
        {
            return true;
        }
        if (const auto* loop = std::get_if<while_statement>(&item->data);
            loop && contains_try(loop->body))
        {
            return true;
        }
        if (const auto* loop = std::get_if<for_loop>(&item->data);
            loop && contains_try(loop->body))
        {
            return true;
        }
        if (const auto* loop = std::get_if<for_each>(&item->data);
            loop && contains_try(loop->body))
        {
            return true;
        }
    }
    return false;
}

void llvm_code_generator::emit_error_exit()
{
    if (!error_targets_.empty())
    {
        const auto& target = error_targets_.back();
        emit_error_cleanup(target.depth);
        body_ << "  br label %" << target.label << '\n';
    }
    else
    {
        emit_error_cleanup(0);
        emit_stack_pop();
        const auto type = llvm_type(return_type_, {});
        // 错误通过状态传播，调用方在读取占位返回值之前跳转。
        body_ << "  ret " << type;
        if (type != "void")
        {
            body_ << (type == "ptr" ? " null" : type == "double" ? " 0.0" : " 0");
        }
        body_ << '\n';
    }
    terminated_ = true;
}

void llvm_code_generator::emit_error_check(const std::string& status)
{
    const auto failed = temporary();
    const auto error = label();
    const auto success = label();
    body_ << "  " << failed << " = icmp ne i32 " << status << ", 0\n"
          << "  br i1 " << failed << ", label %" << error << ", label %" << success << '\n';
    start_block(error);
    emit_error_location();
    emit_error_exit();
    start_block(success);
}

void llvm_code_generator::emit_pending_error_check()
{
    const auto status = temporary();
    body_ << "  " << status << " = load i32, ptr %tx_error_kind\n";
    emit_error_check(status);
}

void llvm_code_generator::emit_try(const try_statement& guarded)
{
    const auto dispatch = label();
    const auto done = label();
    push_scope();
    error_targets_.push_back({dispatch, scopes_.size()});
    emit_statements(guarded.body);
    pop_scope();
    error_targets_.pop_back();
    if (!terminated_)
    {
        write_instruction("br label %" + done);
    }
    start_block(dispatch);
    for (const auto& handler : guarded.handlers)
    {
        const auto selected = label();
        const auto next = label();
        if (handler.kind == error_kind::runtime)
        {
            write_instruction("br label %" + selected);
        }
        else
        {
            const auto kind = temporary();
            const auto matches = temporary();
            body_ << "  " << kind << " = load i32, ptr %tx_error_kind\n";
            write_instruction(matches + " = icmp eq i32 " + kind + ", " +
                              std::to_string(static_cast<int>(handler.kind)));
            write_instruction("br i1 " + matches + ", label %" + selected + ", label %" + next);
        }
        start_block(selected);
        push_scope();
        const auto address = allocate(handler.type, handler.position);
        const auto status = temporary();
        write_instruction(status + " = call i32 @txrt_error_take(ptr " +
                          global_bytes(handler.type.name) + ", ptr " + address + ")");
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
        scopes_.back().emplace(handler.name, variable_slot{handler.type, address});
        emit_statements(handler.body);
        pop_scope();
        if (!terminated_)
        {
            write_instruction("br label %" + done);
        }
        start_block(next);
    }
    emit_error_exit();
    start_block(done);
}

} // namespace tx
