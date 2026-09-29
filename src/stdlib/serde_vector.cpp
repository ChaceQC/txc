#include "stdlib/serde_direct.hpp"
#include "stdlib/vector.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"

namespace tx_generated
{
namespace
{

template<class element_type>
void write_element(serde_writer& writer, const element_type& item,
    const serde_type& type, serde_depth depth)
{
    depth.check(true);
    if constexpr (std::is_same_v<element_type, std::int64_t>)
    {
        writer.integer(item);
    }
    else if constexpr (std::is_same_v<element_type, double>)
    {
        writer.floating(item);
    }
    else if constexpr (std::is_same_v<element_type, std::uint8_t>)
    {
        writer.boolean(item != 0);
    }
    else if constexpr (std::is_same_v<element_type, text_reference>)
    {
        writer.text(item.get());
    }
    else if constexpr (std::is_same_v<element_type, byte_value>)
    {
        writer.bytes(item);
    }
    else
    {
        type.codec->encode(writer, item, type, depth);
    }
}

template<class element_type>
void encode_vector(serde_writer& writer, const std::any& value,
    const serde_type& type, serde_depth depth)
{
    depth.check(true);
    const auto* input = std::any_cast<tx_vector<element_type>>(&value);
    if (!input)
    {
        serde_encode_error("type_mismatch", "serde vector 的实际元素类型不匹配");
    }
    serde_cycle_guard guard(writer.active, input->identity());
    const auto& elements = input->data().values;
    writer.begin(false, elements.size(), depth);
    for (std::size_t index = 0; index < elements.size(); ++index)
    {
        writer.separator(index);
        write_element(writer, elements[index], *type.element, depth.child());
    }
    writer.end(false);
}

template<class element_type>
element_type read_element(serde_reader& reader, const serde_type& type, serde_depth depth)
{
    auto value = type.codec->decode(reader, type, depth);
    if constexpr (std::is_same_v<element_type, std::any>)
    {
        return value;
    }
    else if constexpr (std::is_same_v<element_type, text_reference>)
    {
        // 向量只接收持有文本的引用，绝不保留解析输入的借用地址。
        auto* handle = detail::make_handle<std::string>(std::any_cast<std::string>(std::move(value)));
        text_reference result(handle);
        detail::destroy_handle(handle);
        return result;
    }
    else if constexpr (std::is_same_v<element_type, std::uint8_t>)
    {
        return static_cast<std::uint8_t>(std::any_cast<bool>(value));
    }
    else
    {
        return std::any_cast<element_type>(std::move(value));
    }
}

template<class element_type>
std::any decode_vector(serde_reader& reader, const serde_type& type, serde_depth depth)
{
    auto sequence = reader.begin(false, depth);
    tx_vector<element_type> result(type.name);
    auto& values = result.data().values;
    // 有长度头的 CBOR 只在检查元素限额之后预留；JSON 按实际元素增长。
    values.reserve(static_cast<std::size_t>(sequence.size));
    while (reader.next(sequence))
    {
        values.push_back(read_element<element_type>(reader, *type.element, depth.child()));
    }
    result.data().refresh();
    return result;
}

} // namespace
} // namespace tx_generated

using namespace tx_generated;

extern "C" const serde_codec tx_serde_vector_integer{
    encode_vector<std::int64_t>, decode_vector<std::int64_t>};
extern "C" const serde_codec tx_serde_vector_floating{encode_vector<double>, decode_vector<double>};
extern "C" const serde_codec tx_serde_vector_boolean{
    encode_vector<std::uint8_t>, decode_vector<std::uint8_t>};
extern "C" const serde_codec tx_serde_vector_text{
    encode_vector<text_reference>, decode_vector<text_reference>};
extern "C" const serde_codec tx_serde_vector_bytes{
    encode_vector<byte_value>, decode_vector<byte_value>};
extern "C" const serde_codec tx_serde_vector_object{
    encode_vector<std::any>, decode_vector<std::any>};
