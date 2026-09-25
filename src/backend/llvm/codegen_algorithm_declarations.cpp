#include "backend/llvm/codegen.hpp"

namespace tx
{

void llvm_code_generator::write_algorithm_declarations()
{
    module_ << R"txabi(
declare i32 @txrt_algorithm_sort_i64(ptr)
declare i32 @txrt_algorithm_sorted_i64(ptr, ptr)
declare i32 @txrt_algorithm_find_i64(ptr, i64, ptr)
declare i32 @txrt_algorithm_count_i64(ptr, i64, ptr)
declare i32 @txrt_algorithm_lower_bound_i64(ptr, i64, ptr)
declare i32 @txrt_algorithm_upper_bound_i64(ptr, i64, ptr)
declare i32 @txrt_algorithm_reverse_i64(ptr)
declare i32 @txrt_algorithm_sum_i64(ptr, ptr)
declare i32 @txrt_algorithm_min_element_i64(ptr, ptr)
declare i32 @txrt_algorithm_max_element_i64(ptr, ptr)
declare i32 @txrt_algorithm_sort_f64(ptr)
declare i32 @txrt_algorithm_sorted_f64(ptr, ptr)
declare i32 @txrt_algorithm_find_f64(ptr, double, ptr)
declare i32 @txrt_algorithm_count_f64(ptr, double, ptr)
declare i32 @txrt_algorithm_lower_bound_f64(ptr, double, ptr)
declare i32 @txrt_algorithm_upper_bound_f64(ptr, double, ptr)
declare i32 @txrt_algorithm_reverse_f64(ptr)
declare i32 @txrt_algorithm_sum_f64(ptr, ptr)
declare i32 @txrt_algorithm_min_element_f64(ptr, ptr)
declare i32 @txrt_algorithm_max_element_f64(ptr, ptr)
declare i32 @txrt_algorithm_sort_str(ptr)
declare i32 @txrt_algorithm_sorted_str(ptr, ptr)
declare i32 @txrt_algorithm_find_str(ptr, ptr, ptr)
declare i32 @txrt_algorithm_count_str(ptr, ptr, ptr)
declare i32 @txrt_algorithm_lower_bound_str(ptr, ptr, ptr)
declare i32 @txrt_algorithm_upper_bound_str(ptr, ptr, ptr)
declare i32 @txrt_algorithm_reverse_str(ptr)
declare i32 @txrt_algorithm_find_bool(ptr, i1, ptr)
declare i32 @txrt_algorithm_count_bool(ptr, i1, ptr)
declare i32 @txrt_algorithm_reverse_bool(ptr)
)txabi";
}

} // namespace tx
