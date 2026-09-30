#include "stdlib/serde_direct.hpp"
#include "stdlib/serde_scalar.hpp"

namespace
{

template<class value_type>
void write_field(void* output, const void* storage, const tx_generated::serde_field* field,
    std::uint64_t logical, std::uint64_t wire, std::uint64_t index)
{
    using namespace tx_generated;
    auto& writer = *static_cast<serde_writer*>(output);
    const auto& slot = *static_cast<const typed_slot*>(storage);
    writer.separator(index);
    writer.key(field->name, field->number);
    serde_depth{logical, wire}.check(true);
    if constexpr (std::is_same_v<value_type, std::int64_t>)
    {
        writer.integer(slot.integer);
    }
    else if constexpr (std::is_same_v<value_type, double>)
    {
        writer.floating(slot.floating);
    }
    else if constexpr (std::is_same_v<value_type, bool>)
    {
        writer.boolean(slot.boolean);
    }
    else
    {
        const auto* text = std::any_cast<std::string>(slot.reference);
        if (!text)
        {
            serde_encode_error("type_mismatch", "serde 字段实际类型与静态 schema 不一致");
        }
        writer.text(*text);
    }
}

template<class value_type>
void read_field(void* input, void* storage, const tx_generated::serde_field* field,
    std::uint64_t logical, std::uint64_t wire)
{
    tx_generated::serde_decode_scalar_slot<value_type>(
        *static_cast<tx_generated::serde_reader*>(input),
        *static_cast<tx_generated::typed_slot*>(storage), field->type, {logical, wire});
}

} // namespace

#define TX_SERDE_STATIC(SUFFIX, TYPE) \
extern "C" void tx_serde_write_##SUFFIX(void* writer, const void* slot, \
    const tx_generated::serde_field* field, std::uint64_t logical, std::uint64_t wire, std::uint64_t index) \
{ \
    write_field<TYPE>(writer, slot, field, logical, wire, index); \
} \
extern "C" void tx_serde_read_##SUFFIX(void* reader, void* slot, \
    const tx_generated::serde_field* field, std::uint64_t logical, std::uint64_t wire) \
{ \
    read_field<TYPE>(reader, slot, field, logical, wire); \
}

TX_SERDE_STATIC(i64, std::int64_t)
TX_SERDE_STATIC(f64, double)
TX_SERDE_STATIC(bool, bool)
TX_SERDE_STATIC(str, std::string)

#undef TX_SERDE_STATIC
