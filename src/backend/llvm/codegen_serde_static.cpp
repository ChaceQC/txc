#include "backend/llvm/codegen.hpp"
#include "common/slot_layout.hpp"

#include <algorithm>
#include <numeric>

namespace tx
{
namespace
{

std::string scalar_codec(const value_type& type)
{
    return type == value_type::int_type ? "i64" : type == value_type::float_type ? "f64" :
        type == value_type::bool_type ? "bool" : type == value_type::str_type ? "str" : "";
}

constexpr std::string_view field_layout =
    "{ ptr, i64, i64, { i64, ptr, ptr, ptr, ptr }, { i64, i64, ptr, i64 } }";

} // namespace

bool llvm_code_generator::emit_serde_static(const struct_decl& definition, const std::string& schema)
{
    if (definition.fields.size() > native_record_budget / slot_bytes ||
        !std::all_of(definition.fields.begin(), definition.fields.end(), [](const struct_field& field)
        {
            return field.serde && !field.serde->unknown_capture && !scalar_codec(field.type).empty();
        }))
    {
        return false;
    }
    // 生成代码只消费 C ABI 参数，不读取 C++ reader/writer/dynamic_struct 的私有布局。
    module_ << "define internal void " << schema
            << ".write(ptr %writer, ptr %data, i64 %logical, i64 %wire, i1 %is_json) uwtable {\nentry:\n"
            << "  br i1 %is_json, label %json, label %cbor\n";
    std::vector<std::size_t> order(definition.fields.size());
    std::iota(order.begin(), order.end(), 0);
    for (const bool json : {true, false})
    {
        std::sort(order.begin(), order.end(), [&](std::size_t left, std::size_t right)
        {
            return json ? definition.fields[left].name < definition.fields[right].name :
                definition.fields[left].serde->number < definition.fields[right].serde->number;
        });
        const std::string prefix = json ? "j" : "c";
        module_ << (json ? "json" : "cbor") << ":\n";
        for (std::size_t position = 0; position < order.size(); ++position)
        {
            const auto index = order[position];
            const auto id = prefix + std::to_string(index);
            module_ << "  %slot" << id << " = getelementptr i64, ptr %data, i64 " << index << '\n'
                    << "  %field" << id << " = getelementptr " << field_layout << ", ptr " << schema
                    << ".fields, i64 " << index << '\n'
                    << "  call void @tx_serde_write_" << scalar_codec(definition.fields[index].type)
                    << "(ptr %writer, ptr %slot" << id << ", ptr %field" << id
                    << ", i64 %logical, i64 %wire, i64 " << position + 1 << ")\n";
        }
        module_ << "  ret void\n";
    }
    module_ << "}\n\ndefine internal void " << schema
            << ".read(ptr %reader, ptr %data, i64 %index, i64 %logical, i64 %wire) uwtable {\nentry:\n"
            << "  switch i64 %index, label %invalid [\n";
    for (std::size_t index = 0; index < order.size(); ++index)
    {
        module_ << "    i64 " << index << ", label %field" << index << '\n';
    }
    module_ << "  ]\ninvalid:\n  unreachable\n";
    for (std::size_t index = 0; index < order.size(); ++index)
    {
        module_ << "field" << index << ":\n"
                << "  %slot" << index << " = getelementptr i64, ptr %data, i64 " << index << '\n'
                << "  %metadata" << index << " = getelementptr " << field_layout << ", ptr " << schema
                << ".fields, i64 " << index << '\n'
                << "  call void @tx_serde_read_" << scalar_codec(definition.fields[index].type)
                << "(ptr %reader, ptr %slot" << index << ", ptr %metadata" << index
                << ", i64 %logical, i64 %wire)\n  ret void\n";
    }
    module_ << "}\n\n";
    return true;
}

} // namespace tx
