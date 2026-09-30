#include "backend/cpp/class_local_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/value_format.hpp"
#include "common/local_resource_layout.hpp"

#include <memory>
#include <new>

namespace
{

struct local_class_storage
{
    bool active;
    alignas(tx::local_class_alignment) std::byte data[160];
    tx_generated::record_view view;
};

static_assert(sizeof(local_class_storage) == tx::local_class_bytes);
static_assert(sizeof(tx_generated::record_storage) <= sizeof(local_class_storage::data));
static_assert(alignof(tx_generated::record_storage) <= tx::local_class_alignment);

} // namespace

extern "C" int txrt_class_local_new(const tx_generated::record_type* type,
    void* storage, void** view) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        auto& local = *static_cast<local_class_storage*>(storage);
        auto* record = std::construct_at(reinterpret_cast<tx_generated::record_storage*>(local.data), type);
        try
        {
            for (std::size_t index = 0; index < type->field_count; ++index)
            {
                if (type->fields[index].kind == 0 && type->fields[index].type_name)
                {
                    record->slots.reference(index) = tx_generated::class_default_field(type->fields[index].type_name);
                }
            }
        }
        catch (...)
        {
            std::destroy_at(record);
            throw;
        }
        local.view = {record->slots.data(), type};
        local.active = true;
        *view = &local.view;
    });
}

extern "C" void txrt_class_local_destroy(void* storage,
    void (*destroy)(void*, tx_generated::record_view*)) noexcept
{
    auto& local = *static_cast<local_class_storage*>(storage);
    if (!local.active)
    {
        return;
    }
    local.active = false;
    tx_generated::detail::error_cleanup_guard cleanup_guard;
    auto* record = std::launder(reinterpret_cast<tx_generated::record_storage*>(local.data));
    auto& context = tx_generated::detail::current_runtime_context();
    if (destroy)
    {
        // 编译器生成直接析构调用；此边界仅保护 C++ 字段清理和原错误状态。
        destroy(&context, &local.view);
    }
    std::destroy_at(record);
}
