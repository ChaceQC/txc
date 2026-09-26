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
    for (const auto* suffix : {"i64", "f64", "bool", "str", "bytes", "object"})
    {
        module_ << "declare i32 @txrt_algorithm_stable_sort_" << suffix
                << "(ptr, ptr, ptr)\n"
                << "declare i32 @txrt_algorithm_stable_sorted_" << suffix
                << "(ptr, ptr, ptr, ptr)\n"
                << "declare i32 @txrt_algorithm_binary_search_" << suffix
                << "(ptr, ptr, ptr, ptr, ptr)\n"
                << "declare i32 @txrt_algorithm_equal_range_" << suffix
                << "(ptr, ptr, ptr, ptr, ptr)\n"
                << "declare i32 @txrt_algorithm_unique_" << suffix
                << "(ptr, ptr, ptr, ptr)\n"
                << "declare i32 @txrt_algorithm_rotate_" << suffix
                << "(ptr, i64)\n"
                << "declare i32 @txrt_algorithm_partition_" << suffix
                << "(ptr, ptr, ptr)\n"
                << "declare i32 @txrt_algorithm_filter_" << suffix
                << "(ptr, ptr, ptr)\n"
                << "declare i32 @txrt_algorithm_all_" << suffix
                << "(ptr, ptr, ptr)\n"
                << "declare i32 @txrt_algorithm_any_" << suffix
                << "(ptr, ptr, ptr)\n";
        for (const auto* output : {"i64", "f64", "bool", "str", "bytes", "object"})
        {
            module_ << "declare i32 @txrt_algorithm_map_" << suffix << "_"
                    << output << "(ptr, ptr, ptr, ptr)\n"
                    << "declare i32 @txrt_algorithm_fold_" << suffix << "_"
                    << output << "(ptr, ptr, ptr, ptr)\n";
        }
    }
}

} // namespace tx
