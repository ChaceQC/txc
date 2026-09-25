#include "backend/cpp/runtime_abi_internal.hpp"

namespace tx_generated::detail
{
namespace
{

handle_record<std::string>* text_record(const void* value) noexcept
{
    return static_cast<handle_record<std::string>*>(
        const_cast<std::string*>(static_cast<const std::string*>(value)));
}

} // namespace

std::string* retain_text_handle(const void* value) noexcept
{
    auto* record = text_record(value);
    if (record->references++ == 0)
    {
        register_handle(record, handle_kind::text);
    }
    return record;
}

void retain_text_reference(const void* value) noexcept
{
    if (value)
    {
        ++text_record(value)->internal_references;
    }
}

void release_text_reference(const void* value) noexcept
{
    if (!value)
    {
        return;
    }
    auto* record = text_record(value);
    if (--record->internal_references == 0 && record->references == 0)
    {
        delete record;
    }
}

} // namespace tx_generated::detail
