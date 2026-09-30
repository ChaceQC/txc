#include "backend/llvm/codegen.hpp"

namespace tx
{

void llvm_code_generator::emit_record_operations(const std::string& symbol,
    const std::vector<std::pair<std::string, value_type>>& fields,
    const std::vector<std::size_t>& scans)
{
    module_ << "define internal void " << symbol
            << ".trace(ptr %data, ptr %visit, ptr %context) uwtable {\nentry:\n";
    for (const auto index : scans)
    {
        module_ << "  %slot" << index << " = getelementptr i64, ptr %data, i64 " << index << '\n'
                << "  %reference" << index << " = load ptr, ptr %slot" << index << '\n'
                << "  call void %visit(ptr %reference" << index << ", ptr %context)\n";
    }
    module_ << "  ret void\n}\n\ndefine internal void " << symbol
            << ".copy(ptr %source, ptr %destination, ptr %copy_reference, ptr %context) uwtable {\nentry:\n";
    for (std::size_t index = 0; index < fields.size(); ++index)
    {
        const auto& type = fields[index].second;
        const bool scalar = type == value_type::int_type || type == value_type::float_type ||
            type == value_type::bool_type;
        module_ << "  %from" << index << " = getelementptr i64, ptr %source, i64 " << index << '\n'
                << "  %to" << index << " = getelementptr i64, ptr %destination, i64 " << index << '\n';
        if (scalar)
        {
            module_ << "  %bits" << index << " = load i64, ptr %from" << index << '\n'
                    << "  store i64 %bits" << index << ", ptr %to" << index << '\n';
        }
        else
        {
            // 共享边与循环仍使用同一个复制上下文；只把字段布局展开为直接访问。
            module_ << "  %value" << index << " = load ptr, ptr %from" << index << '\n'
                    << "  %target" << index << " = load ptr, ptr %to" << index << '\n'
                    << "  call void %copy_reference(ptr %value" << index << ", ptr %target" << index
                    << ", i64 " << record_copy_tag(type) << ", ptr %context)\n";
        }
    }
    module_ << "  ret void\n}\n\n";
}

} // namespace tx
