#include "backend/llvm/codegen.hpp"

#include <algorithm>

namespace tx
{

bool llvm_code_generator::static_record_type(const value_type& type) const
{
    if (const auto found = structs_.find(type.name); found != structs_.end())
    {
        return !found->second->native_layout;
    }
    if (const auto found = classes_.find(type.name); found != classes_.end())
    {
        return !found->second->native_layout && std::all_of(
            found->second->bases.begin(), found->second->bases.end(),
            [&](const std::string& base)
            {
                return static_record_type(value_type(base));
            });
    }
    return false;
}

void llvm_code_generator::emit_record_metadata(const value_type& type)
{
    if (!static_record_type(type))
    {
        return;
    }
    const auto symbol = "@tx_record_" + type.name;
    const bool is_class = classes_.contains(type.name);
    std::vector<std::pair<std::string, value_type>> fields;
    std::vector<const class_decl*> nodes;
    std::string display;
    std::size_t destructors = 0;
    if (is_class)
    {
        const auto& definition = *classes_.at(type.name);
        display = definition.source_name;
        fields.resize(class_field_count(definition));
        collect_class_nodes(definition, nodes);
        for (const auto* node : nodes)
        {
            for (const auto& field : node->fields)
            {
                fields[field.slot] = {field.name, field.type};
            }
            destructors += std::count_if(node->methods.begin(), node->methods.end(),
                [](const function_decl& method)
                {
                    return method.name == "deinit";
                });
        }
    }
    else
    {
        const auto& definition = *structs_.at(type.name);
        display = definition.source_name;
        for (const auto& field : definition.fields)
        {
            fields.emplace_back(field.name, field.type);
        }
    }
    if (display.empty())
    {
        display = type.name;
    }
    const auto separator = display.find_last_of('.');
    display = display.substr(separator == std::string::npos ? 0 : separator + 1);
    std::string field_values;
    std::string kinds;
    std::vector<std::size_t> scans;
    for (std::size_t index = 0; index < fields.size(); ++index)
    {
        const auto& [name, field_type] = fields[index];
        const auto kind = field_type == value_type::int_type ? 1 :
            field_type == value_type::float_type ? 2 : field_type == value_type::bool_type ? 3 : 0;
        kinds += static_cast<char>(kind);
        field_values += (index ? ", " : "") + std::string("%tx_record_field { ptr ") +
            (name.empty() ? "null" : global_bytes(name)) + ", ptr " +
            (name.empty() ? "null" : global_bytes(field_type.name)) +
            ", i64 " + std::to_string(index * 8) + ", i64 " + std::to_string(kind) + " }";
        // 不可变文本和 bytes 不构成环；其余引用保守扫描。
        if (!name.empty() && kind == 0 && field_type != value_type::str_type &&
            field_type != value_type::bytes_type)
        {
            scans.push_back(index);
        }
    }
    const auto name = global_bytes(type.name);
    const auto display_name = global_bytes(display);
    const auto kind_data = global_bytes(kinds);
    globals_ << symbol << ".fields = private constant [" << fields.size()
             << " x %tx_record_field] [" << field_values << "]\n"
             << symbol << ".scan = private constant [" << scans.size() << " x i64] [";
    for (std::size_t index = 0; index < scans.size(); ++index)
    {
        globals_ << (index ? ", " : "") << "i64 " << scans[index];
    }
    globals_ << "]\n" << symbol << ".ancestors = private constant [" << nodes.size() << " x ptr] [";
    for (std::size_t index = 0; index < nodes.size(); ++index)
    {
        globals_ << (index ? ", " : "") << "ptr @tx_record_" << nodes[index]->name;
    }
    globals_ << "]\n" << symbol << " = private constant %tx_record_type { ptr " << name
             << ", ptr " << display_name << ", ptr " << symbol << ".fields, i64 " << fields.size()
             << ", ptr " << symbol << ".ancestors, ptr "
             << (is_class ? "@tx_class_ancestors_" + type.name : "null")
             << ", i64 " << nodes.size() << ", ptr "
             << (is_class && virtual_slot_count_ ? "@tx_class_vtable_" + type.name : "null")
             << ", i64 " << (is_class ? virtual_slot_count_ : 0) << ", ptr "
             << (destructors ? "@tx_class_destructors_" + type.name : "null")
             << ", i64 " << destructors << ", ptr " << symbol << ".scan, i64 " << scans.size()
             << ", ptr " << kind_data << " }\n";
}

} // namespace tx
