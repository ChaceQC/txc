#include "backend/llvm/codegen.hpp"

namespace tx
{

void llvm_code_generator::write_ordered_declarations()
{
    for (const auto& [key_suffix, key_abi] : {
             std::pair{"i64", "i64"}, std::pair{"f64", "double"},
             std::pair{"bool", "i1"}, std::pair{"str", "ptr"},
             std::pair{"object", "ptr"}})
    {
        for (const auto& [value_suffix, value_abi] : {
                 std::pair{"i64", "i64"}, std::pair{"f64", "double"},
                 std::pair{"bool", "i1"}, std::pair{"str", "ptr"},
                 std::pair{"object", "ptr"}})
        {
            const std::string name = std::string(key_suffix) + "_" + value_suffix;
            const std::string prefix = "@txrt_ordered_map_";
            module_ << "declare i32 " << prefix << "new_" << name
                    << "(ptr, ptr, ptr, ptr, ptr)\n"
                    << "declare i32 " << prefix << "size_" << name << "(ptr, ptr)\n"
                    << "declare i32 " << prefix << "empty_" << name << "(ptr, ptr)\n"
                    << "declare i32 " << prefix << "clear_" << name << "(ptr)\n"
                    << "declare i32 " << prefix << "contains_" << name
                    << "(ptr, " << key_abi << ", ptr)\n"
                    << "declare i32 " << prefix << "remove_" << name
                    << "(ptr, " << key_abi << ", ptr)\n"
                    << "declare i32 " << prefix << "set_" << name
                    << "(ptr, " << key_abi << ", " << value_abi << ")\n"
                    << "declare i32 " << prefix << "read_" << name
                    << "(ptr, " << key_abi << ", ptr)\n"
                    << "declare i32 " << prefix << "get_" << name
                    << "(ptr, " << key_abi << ", " << value_abi << ", ptr)\n"
                    << "declare i32 " << prefix << "keys_" << name << "(ptr, ptr)\n"
                    << "declare i32 " << prefix << "values_" << name << "(ptr, ptr)\n"
                    << "declare i32 " << prefix << "entries_" << name << "(ptr, ptr)\n"
                    << "declare i32 " << prefix << "range_" << name
                    << "(ptr, " << key_abi << ", " << key_abi << ", ptr)\n";
        }
        const std::string prefix = "@txrt_ordered_set_";
        module_ << "declare i32 " << prefix << "new_" << key_suffix
                << "(ptr, ptr, ptr, ptr)\n"
                << "declare i32 " << prefix << "size_" << key_suffix << "(ptr, ptr)\n"
                << "declare i32 " << prefix << "empty_" << key_suffix << "(ptr, ptr)\n"
                << "declare i32 " << prefix << "clear_" << key_suffix << "(ptr)\n"
                << "declare i32 " << prefix << "contains_" << key_suffix
                << "(ptr, " << key_abi << ", ptr)\n"
                << "declare i32 " << prefix << "remove_" << key_suffix
                << "(ptr, " << key_abi << ", ptr)\n"
                << "declare i32 " << prefix << "insert_" << key_suffix
                << "(ptr, " << key_abi << ", ptr)\n"
                << "declare i32 " << prefix << "to_vector_" << key_suffix
                << "(ptr, ptr)\n"
                << "declare i32 " << prefix << "range_" << key_suffix
                << "(ptr, " << key_abi << ", " << key_abi << ", ptr)\n";
    }
    for (const auto& [suffix, abi] : {
             std::pair{"i64", "i64"}, std::pair{"f64", "double"},
             std::pair{"bool", "i1"}, std::pair{"str", "ptr"},
             std::pair{"object", "ptr"}})
    {
        module_ << "declare i32 @txrt_priority_entry_new_" << suffix
                << "(ptr, i64, " << abi << ", ptr)\n";
    }
}

} // namespace tx
