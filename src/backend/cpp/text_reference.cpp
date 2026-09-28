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

std::string* copy_text_handle(const void* value)
{
    return make_handle<std::string>(*static_cast<const std::string*>(value));
}

void retain_text_reference(const void* value) noexcept
{
    if (value)
    {
        auto* record = text_record(value);
        std::lock_guard lock(record->reference_mutex);
        ++record->internal_references;
    }
}

void release_text_reference(const void* value) noexcept
{
    if (!value)
    {
        return;
    }
    auto* record = text_record(value);
    bool destroy = false;
    {
        std::lock_guard lock(record->reference_mutex);
        --record->internal_references;
        destroy = record->internal_references == 0 && record->references == 0;
    }
    if (destroy)
    {
        delete record;
    }
}

} // namespace tx_generated::detail
