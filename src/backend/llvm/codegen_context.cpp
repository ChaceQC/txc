#include "backend/llvm/codegen.hpp"

namespace tx
{

void llvm_code_generator::write_context_boundary()
{
    module_ << "  %tx_context = call ptr @txrt_runtime_context()\n"
            << "  %tx_error_kind = getelementptr inbounds %tx_runtime_context, "
            << "ptr %tx_context, i32 0, i32 1\n";
}

void llvm_code_generator::write_stack_frame(const function_decl& function)
{
    if (proven_integer_function_ && !profile_mode_)
    {
        // 对整个 0..256 输入域已证明无失败和用户回调，运行时无法观察此诊断帧。
        module_ << "  %tx_error_kind = getelementptr inbounds %tx_runtime_context, ptr %tx_context, i32 0, i32 1\n"
                << "  %tx_bounded_input = icmp ule i64 %arg0, 256\n"
                << "  call void @llvm.assume(i1 %tx_bounded_input)\n";
        return;
    }
    const auto& position = function.position;
    const auto& name = function.source_name.empty() ? function.name : function.source_name;
    module_ << "  %tx_frame = alloca %tx_diagnostic_frame\n"
            << "  %tx_parent_frame = load ptr, ptr %tx_context\n"
            << "  store %tx_diagnostic_frame { ptr " << global_bytes(name)
            << ", ptr " << global_bytes(position.file) << ", i64 "
            << position.line << ", i64 " << position.column
            << ", ptr null }, ptr %tx_frame\n"
            << "  %tx_frame_parent = getelementptr inbounds %tx_diagnostic_frame, "
            << "ptr %tx_frame, i32 0, i32 4\n"
            << "  store ptr %tx_parent_frame, ptr %tx_frame_parent\n"
            << "  store ptr %tx_frame, ptr %tx_context\n"
            << "  %tx_error_kind = getelementptr inbounds %tx_runtime_context, "
            << "ptr %tx_context, i32 0, i32 1\n";
    for (std::size_t index = 1; index <= 3; ++index)
    {
        module_ << "  %tx_location_" << index
                << " = getelementptr inbounds %tx_diagnostic_frame, "
                << "ptr %tx_frame, i32 0, i32 " << index << '\n';
    }
}

void llvm_code_generator::emit_stack_pop()
{
    if (proven_integer_function_ && !profile_mode_)
    {
        return;
    }
    body_ << "  store ptr %tx_parent_frame, ptr %tx_context\n";
}

void llvm_code_generator::emit_stack_location()
{
    if ((proven_integer_function_ && !profile_mode_) || !current_statement_position_)
    {
        return;
    }
    const auto& position = *current_statement_position_;
    if (last_stack_position_ && last_stack_position_->file == position.file &&
        last_stack_position_->line == position.line &&
        last_stack_position_->column == position.column)
    {
        return;
    }
    last_stack_position_ = position;
    body_ << "  store ptr " << global_bytes(position.file) << ", ptr %tx_location_1\n"
          << "  store i64 " << position.line << ", ptr %tx_location_2\n"
          << "  store i64 " << position.column << ", ptr %tx_location_3\n";
}

void llvm_code_generator::emit_error_location()
{
    if (!current_statement_position_)
    {
        return;
    }
    const auto& position = *current_statement_position_;
    body_ << "  call void @txrt_stack_error_location(ptr %tx_context, ptr "
          << global_bytes(position.file) << ", i64 "
          << position.line << ", i64 " << position.column << ")\n";
}

} // namespace tx
