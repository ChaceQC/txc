#pragma once

#include "stdlib/serde_direct.hpp"

namespace tx_generated
{

template<class type>
void serde_decode_scalar_slot(serde_reader& reader, typed_slot& slot, const serde_type&, serde_depth depth)
{
    depth.check(false);
    if constexpr (std::is_same_v<type, std::int64_t>)
    {
        slot.integer = reader.integer(depth.wire);
    }
    else if constexpr (std::is_same_v<type, double>)
    {
        slot.floating = reader.floating(depth.wire);
    }
    else if constexpr (std::is_same_v<type, bool>)
    {
        slot.boolean = reader.boolean(depth.wire);
    }
    else
    {
        slot.reference->emplace<std::string>(reader.text(depth.wire));
    }
}

template<class type>
void serde_write_scalar_slot(serde_writer& writer, const typed_slot& slot, serde_depth depth)
{
    depth.check(true);
    if constexpr (std::is_same_v<type, std::int64_t>)
    {
        writer.integer(slot.integer);
    }
    else if constexpr (std::is_same_v<type, double>)
    {
        writer.floating(slot.floating);
    }
    else if constexpr (std::is_same_v<type, bool>)
    {
        writer.boolean(slot.boolean);
    }
    else
    {
        writer.text(std::any_cast<const std::string&>(*slot.reference));
    }
}

} // namespace tx_generated
