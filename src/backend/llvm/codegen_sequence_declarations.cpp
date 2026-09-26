#include "backend/llvm/codegen.hpp"

namespace tx
{

void llvm_code_generator::write_sequence_declarations()
{
    for (const auto& suffix : {"i64", "f64", "bool", "str", "object"})
    {
        module_ << "declare i32 @txrt_heap_build_" << suffix
                << "(i1, ptr, ptr, ptr, ptr, ptr)\n"
                << "declare i32 @txrt_queue_build_" << suffix
                << "(ptr, ptr, ptr)\n";
    }
    module_ << R"txabi(
declare i32 @txrt_heap_new_object(i1, ptr, ptr, ptr)
declare i32 @txrt_heap_size_object(ptr, ptr)
declare i32 @txrt_heap_empty_object(ptr, ptr)
declare i32 @txrt_heap_clear_object(ptr)
declare i32 @txrt_heap_to_vector_object(ptr, ptr)
declare i32 @txrt_heap_push_object(ptr, ptr)
declare i32 @txrt_heap_pop_object(ptr)
declare i32 @txrt_heap_top_object(ptr, ptr)
declare i32 @txrt_queue_new_object(ptr, ptr)
declare i32 @txrt_queue_size_object(ptr, ptr)
declare i32 @txrt_queue_empty_object(ptr, ptr)
declare i32 @txrt_queue_clear_object(ptr)
declare i32 @txrt_queue_to_vector_object(ptr, ptr)
declare i32 @txrt_queue_push_object(ptr, ptr)
declare i32 @txrt_queue_pop_object(ptr)
declare i32 @txrt_queue_front_object(ptr, ptr)
declare i32 @txrt_queue_back_object(ptr, ptr)
)txabi";
}

} // namespace tx
