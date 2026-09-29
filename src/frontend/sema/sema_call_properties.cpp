#include "frontend/sema/sema.hpp"
#include "frontend/ast/call_properties.hpp"

namespace tx
{

void semantic_analyzer::annotate_call_properties(call_expression& call) const
{
    call.properties = {};
    auto& properties = call.properties;
    properties.arguments.resize(call.arguments.size(), argument_ownership::held);
    if (call.indirect || call.is_constructor)
    {
        return;
    }
    if (!call.receiver && !call.container_type &&
        (call.name == "len" || call.name == "is_none" ||
         call.name == "to_float") && call.arguments.size() == 1)
    {
        properties.effects = {false, false, false, false, false, false};
        if (properties.effects.allows_borrow())
        {
            properties.arguments.front() = argument_ownership::borrowed;
        }
        return;
    }
    if (call.receiver || call.container_type)
    {
        const auto& type = call.container_type ? *call.container_type : call.receiver->type;
        properties.effects = container_call_effects(type,
            call.container_type ? "new" : call.name);
        if (call.receiver && properties.effects.allows_borrow())
        {
            properties.receiver = argument_ownership::borrowed;
        }
        return;
    }
    const auto found = functions_.find(call.name);
    if (!call.overload_index || found == functions_.end())
    {
        return;
    }
    const auto& signature = found->second.at(*call.overload_index);
    if (!signature.external)
    {
        return;
    }
    const auto& name = signature.external_name;
    // 这些原生入口只在调用期间读取参数；结果/错误仍可分配，但不保存借用地址。
    const bool read_only_input = name == "parse.try_parse_int" ||
        name == "parse.try_parse_float" || name == "parse.parse_int" ||
        name == "parse.parse_float" || name == "encoding.encode" ||
        name == "encoding.decode" || name == "bytes.from_hex" ||
        name == "bytes.from_base64" || name == "bytes.from_base64_url" ||
        name == "bytes.to_hex" || name == "bytes.to_base64" ||
        name == "bytes.to_base64_url" || name == "bytes.to_hex_chunk" ||
        name == "bytes.to_base64_chunk";
    if (read_only_input)
    {
        properties.effects = {true, false, false, false, false, false};
        for (auto& ownership : properties.arguments)
        {
            ownership = argument_ownership::borrowed;
        }
        return;
    }
    const bool dictionary_query = (name == "dictionary.get" ||
        name == "dictionary.contains") && call.arguments.size() == 2;
    const bool cancel_query = name == "cancel.status";
    if (dictionary_query || cancel_query)
    {
        properties.effects = {name == "dictionary.get", false, false,
                              false, false, false};
        for (auto& ownership : properties.arguments)
        {
            ownership = argument_ownership::borrowed;
        }
        return;
    }
    if (name == "math.sqrt" || name == "random.seed" ||
        name == "random.random_int" || name == "random.random_float")
    {
        properties.effects = {false, false, false, false, false, false};
    }
}

} // namespace tx
