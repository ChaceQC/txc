#pragma once

#include "backend/cpp/format_abi.hpp"
#include "stdlib/xml.hpp"

namespace tx_generated::xml_abi
{

inline xml_limits read_limits(const void* value)
{
    using format_abi::member;
    return {member<std::int64_t>(value, 0), member<std::int64_t>(value, 1),
        member<std::int64_t>(value, 2), member<std::int64_t>(value, 3),
        member<std::int64_t>(value, 4)};
}

inline void* node_handle(xml_node node)
{
    return detail::make_handle<std::any>(std::move(node));
}

inline void* document_handle(xml_document document)
{
    return detail::make_handle<std::any>(std::move(document));
}

} // namespace tx_generated::xml_abi
