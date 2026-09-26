#include "backend/llvm/codegen.hpp"

namespace tx
{

void llvm_code_generator::write_container_declarations()
{
    module_ << R"txabi(
declare i32 @txrt_map_new_i64_i64(ptr)
declare i32 @txrt_map_size_i64_i64(ptr, ptr)
declare i32 @txrt_map_empty_i64_i64(ptr, ptr)
declare i32 @txrt_map_clear_i64_i64(ptr)
declare i32 @txrt_map_contains_i64_i64(ptr, i64, ptr)
declare i32 @txrt_map_remove_i64_i64(ptr, i64, ptr)
declare i32 @txrt_map_set_i64_i64(ptr, i64, i64)
declare i32 @txrt_map_read_i64_i64(ptr, i64, ptr)
declare i32 @txrt_map_get_i64_i64(ptr, i64, i64, ptr)
declare i32 @txrt_map_keys_i64_i64(ptr, ptr)
declare i32 @txrt_map_values_i64_i64(ptr, ptr)
declare i32 @txrt_map_new_i64_f64(ptr)
declare i32 @txrt_map_size_i64_f64(ptr, ptr)
declare i32 @txrt_map_empty_i64_f64(ptr, ptr)
declare i32 @txrt_map_clear_i64_f64(ptr)
declare i32 @txrt_map_contains_i64_f64(ptr, i64, ptr)
declare i32 @txrt_map_remove_i64_f64(ptr, i64, ptr)
declare i32 @txrt_map_set_i64_f64(ptr, i64, double)
declare i32 @txrt_map_read_i64_f64(ptr, i64, ptr)
declare i32 @txrt_map_get_i64_f64(ptr, i64, double, ptr)
declare i32 @txrt_map_keys_i64_f64(ptr, ptr)
declare i32 @txrt_map_values_i64_f64(ptr, ptr)
declare i32 @txrt_map_new_i64_bool(ptr)
declare i32 @txrt_map_size_i64_bool(ptr, ptr)
declare i32 @txrt_map_empty_i64_bool(ptr, ptr)
declare i32 @txrt_map_clear_i64_bool(ptr)
declare i32 @txrt_map_contains_i64_bool(ptr, i64, ptr)
declare i32 @txrt_map_remove_i64_bool(ptr, i64, ptr)
declare i32 @txrt_map_set_i64_bool(ptr, i64, i1)
declare i32 @txrt_map_read_i64_bool(ptr, i64, ptr)
declare i32 @txrt_map_get_i64_bool(ptr, i64, i1, ptr)
declare i32 @txrt_map_keys_i64_bool(ptr, ptr)
declare i32 @txrt_map_values_i64_bool(ptr, ptr)
declare i32 @txrt_map_new_i64_str(ptr)
declare i32 @txrt_map_size_i64_str(ptr, ptr)
declare i32 @txrt_map_empty_i64_str(ptr, ptr)
declare i32 @txrt_map_clear_i64_str(ptr)
declare i32 @txrt_map_contains_i64_str(ptr, i64, ptr)
declare i32 @txrt_map_remove_i64_str(ptr, i64, ptr)
declare i32 @txrt_map_set_i64_str(ptr, i64, ptr)
declare i32 @txrt_map_read_i64_str(ptr, i64, ptr)
declare i32 @txrt_map_get_i64_str(ptr, i64, ptr, ptr)
declare i32 @txrt_map_keys_i64_str(ptr, ptr)
declare i32 @txrt_map_values_i64_str(ptr, ptr)
declare i32 @txrt_map_new_f64_i64(ptr)
declare i32 @txrt_map_size_f64_i64(ptr, ptr)
declare i32 @txrt_map_empty_f64_i64(ptr, ptr)
declare i32 @txrt_map_clear_f64_i64(ptr)
declare i32 @txrt_map_contains_f64_i64(ptr, double, ptr)
declare i32 @txrt_map_remove_f64_i64(ptr, double, ptr)
declare i32 @txrt_map_set_f64_i64(ptr, double, i64)
declare i32 @txrt_map_read_f64_i64(ptr, double, ptr)
declare i32 @txrt_map_get_f64_i64(ptr, double, i64, ptr)
declare i32 @txrt_map_keys_f64_i64(ptr, ptr)
declare i32 @txrt_map_values_f64_i64(ptr, ptr)
declare i32 @txrt_map_new_f64_f64(ptr)
declare i32 @txrt_map_size_f64_f64(ptr, ptr)
declare i32 @txrt_map_empty_f64_f64(ptr, ptr)
declare i32 @txrt_map_clear_f64_f64(ptr)
declare i32 @txrt_map_contains_f64_f64(ptr, double, ptr)
declare i32 @txrt_map_remove_f64_f64(ptr, double, ptr)
declare i32 @txrt_map_set_f64_f64(ptr, double, double)
declare i32 @txrt_map_read_f64_f64(ptr, double, ptr)
declare i32 @txrt_map_get_f64_f64(ptr, double, double, ptr)
declare i32 @txrt_map_keys_f64_f64(ptr, ptr)
declare i32 @txrt_map_values_f64_f64(ptr, ptr)
declare i32 @txrt_map_new_f64_bool(ptr)
declare i32 @txrt_map_size_f64_bool(ptr, ptr)
declare i32 @txrt_map_empty_f64_bool(ptr, ptr)
declare i32 @txrt_map_clear_f64_bool(ptr)
declare i32 @txrt_map_contains_f64_bool(ptr, double, ptr)
declare i32 @txrt_map_remove_f64_bool(ptr, double, ptr)
declare i32 @txrt_map_set_f64_bool(ptr, double, i1)
declare i32 @txrt_map_read_f64_bool(ptr, double, ptr)
declare i32 @txrt_map_get_f64_bool(ptr, double, i1, ptr)
declare i32 @txrt_map_keys_f64_bool(ptr, ptr)
declare i32 @txrt_map_values_f64_bool(ptr, ptr)
declare i32 @txrt_map_new_f64_str(ptr)
declare i32 @txrt_map_size_f64_str(ptr, ptr)
declare i32 @txrt_map_empty_f64_str(ptr, ptr)
declare i32 @txrt_map_clear_f64_str(ptr)
declare i32 @txrt_map_contains_f64_str(ptr, double, ptr)
declare i32 @txrt_map_remove_f64_str(ptr, double, ptr)
declare i32 @txrt_map_set_f64_str(ptr, double, ptr)
declare i32 @txrt_map_read_f64_str(ptr, double, ptr)
declare i32 @txrt_map_get_f64_str(ptr, double, ptr, ptr)
declare i32 @txrt_map_keys_f64_str(ptr, ptr)
declare i32 @txrt_map_values_f64_str(ptr, ptr)
declare i32 @txrt_map_new_bool_i64(ptr)
declare i32 @txrt_map_size_bool_i64(ptr, ptr)
declare i32 @txrt_map_empty_bool_i64(ptr, ptr)
declare i32 @txrt_map_clear_bool_i64(ptr)
declare i32 @txrt_map_contains_bool_i64(ptr, i1, ptr)
declare i32 @txrt_map_remove_bool_i64(ptr, i1, ptr)
declare i32 @txrt_map_set_bool_i64(ptr, i1, i64)
declare i32 @txrt_map_read_bool_i64(ptr, i1, ptr)
declare i32 @txrt_map_get_bool_i64(ptr, i1, i64, ptr)
declare i32 @txrt_map_keys_bool_i64(ptr, ptr)
declare i32 @txrt_map_values_bool_i64(ptr, ptr)
declare i32 @txrt_map_new_bool_f64(ptr)
declare i32 @txrt_map_size_bool_f64(ptr, ptr)
declare i32 @txrt_map_empty_bool_f64(ptr, ptr)
declare i32 @txrt_map_clear_bool_f64(ptr)
declare i32 @txrt_map_contains_bool_f64(ptr, i1, ptr)
declare i32 @txrt_map_remove_bool_f64(ptr, i1, ptr)
declare i32 @txrt_map_set_bool_f64(ptr, i1, double)
declare i32 @txrt_map_read_bool_f64(ptr, i1, ptr)
declare i32 @txrt_map_get_bool_f64(ptr, i1, double, ptr)
declare i32 @txrt_map_keys_bool_f64(ptr, ptr)
declare i32 @txrt_map_values_bool_f64(ptr, ptr)
declare i32 @txrt_map_new_bool_bool(ptr)
declare i32 @txrt_map_size_bool_bool(ptr, ptr)
declare i32 @txrt_map_empty_bool_bool(ptr, ptr)
declare i32 @txrt_map_clear_bool_bool(ptr)
declare i32 @txrt_map_contains_bool_bool(ptr, i1, ptr)
declare i32 @txrt_map_remove_bool_bool(ptr, i1, ptr)
declare i32 @txrt_map_set_bool_bool(ptr, i1, i1)
declare i32 @txrt_map_read_bool_bool(ptr, i1, ptr)
declare i32 @txrt_map_get_bool_bool(ptr, i1, i1, ptr)
declare i32 @txrt_map_keys_bool_bool(ptr, ptr)
declare i32 @txrt_map_values_bool_bool(ptr, ptr)
declare i32 @txrt_map_new_bool_str(ptr)
declare i32 @txrt_map_size_bool_str(ptr, ptr)
declare i32 @txrt_map_empty_bool_str(ptr, ptr)
declare i32 @txrt_map_clear_bool_str(ptr)
declare i32 @txrt_map_contains_bool_str(ptr, i1, ptr)
declare i32 @txrt_map_remove_bool_str(ptr, i1, ptr)
declare i32 @txrt_map_set_bool_str(ptr, i1, ptr)
declare i32 @txrt_map_read_bool_str(ptr, i1, ptr)
declare i32 @txrt_map_get_bool_str(ptr, i1, ptr, ptr)
declare i32 @txrt_map_keys_bool_str(ptr, ptr)
declare i32 @txrt_map_values_bool_str(ptr, ptr)
declare i32 @txrt_map_new_str_i64(ptr)
declare i32 @txrt_map_size_str_i64(ptr, ptr)
declare i32 @txrt_map_empty_str_i64(ptr, ptr)
declare i32 @txrt_map_clear_str_i64(ptr)
declare i32 @txrt_map_contains_str_i64(ptr, ptr, ptr)
declare i32 @txrt_map_remove_str_i64(ptr, ptr, ptr)
declare i32 @txrt_map_set_str_i64(ptr, ptr, i64)
declare i32 @txrt_map_read_str_i64(ptr, ptr, ptr)
declare i32 @txrt_map_get_str_i64(ptr, ptr, i64, ptr)
declare i32 @txrt_map_keys_str_i64(ptr, ptr)
declare i32 @txrt_map_values_str_i64(ptr, ptr)
declare i32 @txrt_map_new_str_f64(ptr)
declare i32 @txrt_map_size_str_f64(ptr, ptr)
declare i32 @txrt_map_empty_str_f64(ptr, ptr)
declare i32 @txrt_map_clear_str_f64(ptr)
declare i32 @txrt_map_contains_str_f64(ptr, ptr, ptr)
declare i32 @txrt_map_remove_str_f64(ptr, ptr, ptr)
declare i32 @txrt_map_set_str_f64(ptr, ptr, double)
declare i32 @txrt_map_read_str_f64(ptr, ptr, ptr)
declare i32 @txrt_map_get_str_f64(ptr, ptr, double, ptr)
declare i32 @txrt_map_keys_str_f64(ptr, ptr)
declare i32 @txrt_map_values_str_f64(ptr, ptr)
declare i32 @txrt_map_new_str_bool(ptr)
declare i32 @txrt_map_size_str_bool(ptr, ptr)
declare i32 @txrt_map_empty_str_bool(ptr, ptr)
declare i32 @txrt_map_clear_str_bool(ptr)
declare i32 @txrt_map_contains_str_bool(ptr, ptr, ptr)
declare i32 @txrt_map_remove_str_bool(ptr, ptr, ptr)
declare i32 @txrt_map_set_str_bool(ptr, ptr, i1)
declare i32 @txrt_map_read_str_bool(ptr, ptr, ptr)
declare i32 @txrt_map_get_str_bool(ptr, ptr, i1, ptr)
declare i32 @txrt_map_keys_str_bool(ptr, ptr)
declare i32 @txrt_map_values_str_bool(ptr, ptr)
declare i32 @txrt_map_new_str_str(ptr)
declare i32 @txrt_map_size_str_str(ptr, ptr)
declare i32 @txrt_map_empty_str_str(ptr, ptr)
declare i32 @txrt_map_clear_str_str(ptr)
declare i32 @txrt_map_contains_str_str(ptr, ptr, ptr)
declare i32 @txrt_map_remove_str_str(ptr, ptr, ptr)
declare i32 @txrt_map_set_str_str(ptr, ptr, ptr)
declare i32 @txrt_map_read_str_str(ptr, ptr, ptr)
declare i32 @txrt_map_get_str_str(ptr, ptr, ptr, ptr)
declare i32 @txrt_map_keys_str_str(ptr, ptr)
declare i32 @txrt_map_values_str_str(ptr, ptr)
declare i32 @txrt_set_new_i64(ptr)
declare i32 @txrt_set_size_i64(ptr, ptr)
declare i32 @txrt_set_empty_i64(ptr, ptr)
declare i32 @txrt_set_clear_i64(ptr)
declare i32 @txrt_set_contains_i64(ptr, i64, ptr)
declare i32 @txrt_set_remove_i64(ptr, i64, ptr)
declare i32 @txrt_set_to_vector_i64(ptr, ptr)
declare i32 @txrt_set_insert_i64(ptr, i64, ptr)
declare i32 @txrt_set_new_f64(ptr)
declare i32 @txrt_set_size_f64(ptr, ptr)
declare i32 @txrt_set_empty_f64(ptr, ptr)
declare i32 @txrt_set_clear_f64(ptr)
declare i32 @txrt_set_contains_f64(ptr, double, ptr)
declare i32 @txrt_set_remove_f64(ptr, double, ptr)
declare i32 @txrt_set_to_vector_f64(ptr, ptr)
declare i32 @txrt_set_insert_f64(ptr, double, ptr)
declare i32 @txrt_set_new_bool(ptr)
declare i32 @txrt_set_size_bool(ptr, ptr)
declare i32 @txrt_set_empty_bool(ptr, ptr)
declare i32 @txrt_set_clear_bool(ptr)
declare i32 @txrt_set_contains_bool(ptr, i1, ptr)
declare i32 @txrt_set_remove_bool(ptr, i1, ptr)
declare i32 @txrt_set_to_vector_bool(ptr, ptr)
declare i32 @txrt_set_insert_bool(ptr, i1, ptr)
declare i32 @txrt_set_new_str(ptr)
declare i32 @txrt_set_size_str(ptr, ptr)
declare i32 @txrt_set_empty_str(ptr, ptr)
declare i32 @txrt_set_clear_str(ptr)
declare i32 @txrt_set_contains_str(ptr, ptr, ptr)
declare i32 @txrt_set_remove_str(ptr, ptr, ptr)
declare i32 @txrt_set_to_vector_str(ptr, ptr)
declare i32 @txrt_set_insert_str(ptr, ptr, ptr)
declare i32 @txrt_heap_new_i64(i1, ptr)
declare i32 @txrt_heap_size_i64(ptr, ptr)
declare i32 @txrt_heap_empty_i64(ptr, ptr)
declare i32 @txrt_heap_clear_i64(ptr)
declare i32 @txrt_heap_to_vector_i64(ptr, ptr)
declare i32 @txrt_heap_push_i64(ptr, i64)
declare i32 @txrt_heap_pop_i64(ptr)
declare i32 @txrt_heap_top_i64(ptr, ptr)
declare i32 @txrt_heap_new_f64(i1, ptr)
declare i32 @txrt_heap_size_f64(ptr, ptr)
declare i32 @txrt_heap_empty_f64(ptr, ptr)
declare i32 @txrt_heap_clear_f64(ptr)
declare i32 @txrt_heap_to_vector_f64(ptr, ptr)
declare i32 @txrt_heap_push_f64(ptr, double)
declare i32 @txrt_heap_pop_f64(ptr)
declare i32 @txrt_heap_top_f64(ptr, ptr)
declare i32 @txrt_heap_new_bool(i1, ptr)
declare i32 @txrt_heap_size_bool(ptr, ptr)
declare i32 @txrt_heap_empty_bool(ptr, ptr)
declare i32 @txrt_heap_clear_bool(ptr)
declare i32 @txrt_heap_to_vector_bool(ptr, ptr)
declare i32 @txrt_heap_push_bool(ptr, i1)
declare i32 @txrt_heap_pop_bool(ptr)
declare i32 @txrt_heap_top_bool(ptr, ptr)
declare i32 @txrt_heap_new_str(i1, ptr)
declare i32 @txrt_heap_size_str(ptr, ptr)
declare i32 @txrt_heap_empty_str(ptr, ptr)
declare i32 @txrt_heap_clear_str(ptr)
declare i32 @txrt_heap_to_vector_str(ptr, ptr)
declare i32 @txrt_heap_push_str(ptr, ptr)
declare i32 @txrt_heap_pop_str(ptr)
declare i32 @txrt_heap_top_str(ptr, ptr)
declare i32 @txrt_queue_new_i64(ptr)
declare i32 @txrt_queue_size_i64(ptr, ptr)
declare i32 @txrt_queue_empty_i64(ptr, ptr)
declare i32 @txrt_queue_clear_i64(ptr)
declare i32 @txrt_queue_to_vector_i64(ptr, ptr)
declare i32 @txrt_queue_push_i64(ptr, i64)
declare i32 @txrt_queue_pop_i64(ptr)
declare i32 @txrt_queue_front_i64(ptr, ptr)
declare i32 @txrt_queue_back_i64(ptr, ptr)
declare i32 @txrt_queue_new_f64(ptr)
declare i32 @txrt_queue_size_f64(ptr, ptr)
declare i32 @txrt_queue_empty_f64(ptr, ptr)
declare i32 @txrt_queue_clear_f64(ptr)
declare i32 @txrt_queue_to_vector_f64(ptr, ptr)
declare i32 @txrt_queue_push_f64(ptr, double)
declare i32 @txrt_queue_pop_f64(ptr)
declare i32 @txrt_queue_front_f64(ptr, ptr)
declare i32 @txrt_queue_back_f64(ptr, ptr)
declare i32 @txrt_queue_new_bool(ptr)
declare i32 @txrt_queue_size_bool(ptr, ptr)
declare i32 @txrt_queue_empty_bool(ptr, ptr)
declare i32 @txrt_queue_clear_bool(ptr)
declare i32 @txrt_queue_to_vector_bool(ptr, ptr)
declare i32 @txrt_queue_push_bool(ptr, i1)
declare i32 @txrt_queue_pop_bool(ptr)
declare i32 @txrt_queue_front_bool(ptr, ptr)
declare i32 @txrt_queue_back_bool(ptr, ptr)
declare i32 @txrt_queue_new_str(ptr)
declare i32 @txrt_queue_size_str(ptr, ptr)
declare i32 @txrt_queue_empty_str(ptr, ptr)
declare i32 @txrt_queue_clear_str(ptr)
declare i32 @txrt_queue_to_vector_str(ptr, ptr)
declare i32 @txrt_queue_push_str(ptr, ptr)
declare i32 @txrt_queue_pop_str(ptr)
declare i32 @txrt_queue_front_str(ptr, ptr)
declare i32 @txrt_queue_back_str(ptr, ptr)
)txabi";
    for (const auto& [suffix, abi_type] : {
             std::pair{"i64", "i64"}, std::pair{"f64", "double"},
             std::pair{"bool", "i1"}, std::pair{"str", "ptr"}})
    {
        const std::string name = std::string("object_") + suffix;
        const std::string prefix = "@txrt_map_";
        module_ << "declare i32 " << prefix << "new_" << name
                << "(ptr, ptr, ptr, ptr)\n"
                << "declare i32 " << prefix << "size_" << name << "(ptr, ptr)\n"
                << "declare i32 " << prefix << "empty_" << name << "(ptr, ptr)\n"
                << "declare i32 " << prefix << "clear_" << name << "(ptr)\n"
                << "declare i32 " << prefix << "contains_" << name << "(ptr, ptr, ptr)\n"
                << "declare i32 " << prefix << "remove_" << name << "(ptr, ptr, ptr)\n"
                << "declare i32 " << prefix << "set_" << name
                << "(ptr, ptr, " << abi_type << ")\n"
                << "declare i32 " << prefix << "read_" << name << "(ptr, ptr, ptr)\n"
                << "declare i32 " << prefix << "get_" << name
                << "(ptr, ptr, " << abi_type << ", ptr)\n"
                << "declare i32 " << prefix << "keys_" << name << "(ptr, ptr)\n"
                << "declare i32 " << prefix << "values_" << name << "(ptr, ptr)\n"
                << "declare i32 " << prefix << "entries_" << name << "(ptr, ptr)\n";
    }
    for (const auto& [key_suffix, key_abi] : {
             std::pair{"i64", "i64"}, std::pair{"f64", "double"},
             std::pair{"bool", "i1"}, std::pair{"str", "ptr"},
             std::pair{"object", "ptr"}})
    {
        const std::string prefix = "@txrt_map_";
        for (const auto& value_suffix : {"i64", "f64", "bool", "str"})
        {
            if (std::string_view(key_suffix) != "object")
            {
                module_ << "declare i32 " << prefix << "entries_" << key_suffix
                        << '_' << value_suffix << "(ptr, ptr)\n";
            }
        }
        const std::string name = std::string(key_suffix) + "_object";
        module_ << "declare i32 " << prefix << "new_" << name
                << (std::string_view(key_suffix) == "object"
                    ? "(ptr, ptr, ptr, ptr, ptr)\n" : "(ptr, ptr)\n")
                << "declare i32 " << prefix << "size_" << name << "(ptr, ptr)\n"
                << "declare i32 " << prefix << "empty_" << name << "(ptr, ptr)\n"
                << "declare i32 " << prefix << "clear_" << name << "(ptr)\n"
                << "declare i32 " << prefix << "contains_" << name
                << "(ptr, " << key_abi << ", ptr)\n"
                << "declare i32 " << prefix << "remove_" << name
                << "(ptr, " << key_abi << ", ptr)\n"
                << "declare i32 " << prefix << "set_" << name
                << "(ptr, " << key_abi << ", ptr)\n"
                << "declare i32 " << prefix << "read_" << name
                << "(ptr, " << key_abi << ", ptr)\n"
                << "declare i32 " << prefix << "get_" << name
                << "(ptr, " << key_abi << ", ptr, ptr)\n"
                << "declare i32 " << prefix << "keys_" << name << "(ptr, ptr)\n"
                << "declare i32 " << prefix << "values_" << name << "(ptr, ptr)\n"
                << "declare i32 " << prefix << "entries_" << name << "(ptr, ptr)\n";
    }
    module_ << R"txabi(
declare i32 @txrt_set_new_object(ptr, ptr, ptr, ptr)
declare i32 @txrt_set_size_object(ptr, ptr)
declare i32 @txrt_set_empty_object(ptr, ptr)
declare i32 @txrt_set_clear_object(ptr)
declare i32 @txrt_set_contains_object(ptr, ptr, ptr)
declare i32 @txrt_set_remove_object(ptr, ptr, ptr)
declare i32 @txrt_set_insert_object(ptr, ptr, ptr)
declare i32 @txrt_set_to_vector_object(ptr, ptr)
)txabi";
    for (const auto& [suffix, abi_type] : {
             std::pair{"i64", "i64"}, std::pair{"f64", "double"},
             std::pair{"bool", "i1"}, std::pair{"str", "ptr"},
             std::pair{"object", "ptr"}})
    {
        const std::string prefix = "@txrt_deque_";
        const std::string name = suffix;
        module_ << "declare i32 " << prefix << "new_" << name
                << (name == "object" ? "(ptr, ptr)\n" : "(ptr)\n")
                << "declare i32 " << prefix << "size_" << name << "(ptr, ptr)\n"
                << "declare i32 " << prefix << "empty_" << name << "(ptr, ptr)\n"
                << "declare i32 " << prefix << "clear_" << name << "(ptr)\n"
                << "declare i32 " << prefix << "to_vector_" << name << "(ptr, ptr)\n"
                << "declare i32 " << prefix << "push_front_" << name
                << "(ptr, " << abi_type << ")\n"
                << "declare i32 " << prefix << "push_back_" << name
                << "(ptr, " << abi_type << ")\n"
                << "declare i32 " << prefix << "pop_front_" << name << "(ptr)\n"
                << "declare i32 " << prefix << "pop_back_" << name << "(ptr)\n"
                << "declare i32 " << prefix << "front_" << name << "(ptr, ptr)\n"
                << "declare i32 " << prefix << "back_" << name << "(ptr, ptr)\n"
                << "declare i32 " << prefix << "read_" << name << "(ptr, i64, ptr)\n"
                << "declare i32 " << prefix << "set_" << name
                << "(ptr, i64, " << abi_type << ")\n"
                << "declare i32 " << prefix << "insert_" << name
                << "(ptr, i64, " << abi_type << ")\n"
                << "declare i32 " << prefix << "erase_" << name << "(ptr, i64)\n";
    }
    write_ordered_declarations();
    write_sequence_declarations();
}

} // namespace tx
