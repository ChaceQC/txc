#include "backend/cpp/record_layout.hpp"

namespace tx_generated
{

record_storage::record_storage(const record_type* description) : type(description)
{
    slots = typed_slots({type->kinds, type->field_count}, true);
}

void record_storage::scan(gc_visit visit, void* context) const
{
    if (type->trace_slots)
    {
        type->trace_slots(slots.data(), visit, context);
        return;
    }
    for (std::size_t index = 0; index < type->scan_count; ++index)
    {
        visit(slots.reference(type->scan_indices[index]), context);
    }
}

} // namespace tx_generated
