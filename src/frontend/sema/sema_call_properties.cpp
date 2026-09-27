#include "frontend/sema/sema.hpp"
#include "frontend/ast/call_properties.hpp"

namespace tx
{

void semantic_analyzer::annotate_call_properties(call_expression& call) const
{
    call.properties = {};
    auto& properties = call.properties;
    properties.arguments.resize(call.arguments.size(), argument_ownership::held);
    if (call.name == "len" && !call.receiver && call.arguments.size() == 1)
    {
        properties.effects = container_call_effects(call.arguments.front().value->type, "size");
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
    const bool dictionary_query = name == "dictionary.get" ||
        name == "dictionary.contains";
    const bool cancel_query = name == "cancel.status";
    if (dictionary_query || cancel_query)
    {
        properties.effects = {name == "dictionary.get", false, false, false, false};
        for (auto& ownership : properties.arguments)
        {
            ownership = argument_ownership::borrowed;
        }
    }
}

} // namespace tx
