#include "backend/cpp/value_abi.hpp"

#include "backend/cpp/runtime.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/value_format.hpp"

#include <any>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace tx_generated
{

namespace
{
bool finalize_class_object(const std::shared_ptr<dynamic_class>& object)
{
    if (!object || object->destroying)
    {
        return false;
    }
    object->destroying = true;
    for (std::size_t index = 0; index < object->destructor_count; ++index)
    {
        // 接收者由生成方法持有并释放；回调期间额外引用保证字段仍然有效。
        auto* receiver = detail::make_handle<std::any>(class_handle(object));
        using destructor_fn = void (*)(void*);
        auto callback = reinterpret_cast<destructor_fn>(
            const_cast<void*>(object->destructor_targets[index]));
        callback(receiver);
    }
    return true;
}
} // namespace

class_handle::~class_handle()
{
    if (object_ && object_.use_count() == 1)
    {
        (void)finalize_class_object(object_);
    }
}

void register_class_gc(const std::shared_ptr<dynamic_class>& object)
{
    register_gc_node(object,
        [](const void* value, gc_visit visit, void* context)
        {
            for (const auto& field : static_cast<const dynamic_class*>(value)->fields)
            {
                visit(field, context);
            }
        },
        [](void* value)
        {
            static_cast<dynamic_class*>(value)->fields.clear();
        },
        [](const std::shared_ptr<void>& value)
        {
            return finalize_class_object(
                std::static_pointer_cast<dynamic_class>(value));
        });
}

} // namespace tx_generated

namespace
{

tx_generated::dynamic_class& as_class(const void* value)
{
    const auto& item = *static_cast<const std::any*>(value);
    if (item.type() != typeid(tx_generated::class_handle))
    {
        throw std::runtime_error("成员访问需要 class 对象");
    }
    const auto& handle = std::any_cast<const tx_generated::class_handle&>(item);
    if (!handle)
    {
        throw std::runtime_error("class 对象引用为空");
    }
    return *handle;
}

std::any default_field(const std::string& type)
{
    if (type == "int")
    {
        return std::int64_t{0};
    }
    if (type == "float")
    {
        return 0.0;
    }
    if (type == "bool")
    {
        return false;
    }
    if (type == "str")
    {
        return std::string{};
    }
    if (type == "array")
    {
        return tx_generated::tx_array{};
    }
    if (type == "dict")
    {
        return tx_generated::tx_dict{};
    }
    return {};
}

} // namespace

using tx_generated::detail::invoke_checked;

extern "C" int txrt_class_new(
    const char* type_name, const char* display_name,
    const char* const* ancestors, std::size_t ancestor_count,
    const char* const* field_types, std::size_t field_count,
    const void* const* virtual_targets, std::size_t virtual_count,
    const void* const* destructor_targets, std::size_t destructor_count,
    void** result) noexcept
{
    return invoke_checked([&]
    {
        auto object = std::make_shared<tx_generated::dynamic_class>();
        object->type_name = type_name;
        object->display_name = display_name;
        for (std::size_t i = 0; i < ancestor_count; ++i)
        {
            object->ancestors.emplace_back(ancestors[i]);
        }
        object->fields.reserve(field_count);
        for (std::size_t i = 0; i < field_count; ++i)
        {
            object->fields.push_back(field_types[i] == nullptr
                ? std::any{} : default_field(field_types[i]));
        }
        object->virtual_targets = virtual_targets;
        object->virtual_count = virtual_count;
        object->destructor_targets = destructor_targets;
        object->destructor_count = destructor_count;
        tx_generated::register_class_gc(object);
        *result = tx_generated::detail::make_handle<std::any>(
            tx_generated::class_handle(std::move(object)));
    });
}

extern "C" int txrt_class_field_address_index(
    void* value, std::size_t index, bool for_write, void** result) noexcept
{
    return invoke_checked([&]
    {
        auto& object = as_class(value);
        if (index >= object.fields.size())
        {
            throw std::runtime_error("类字段索引越界");
        }
        auto& field = object.fields[index];
        if (!for_write && !field.has_value())
        {
            throw std::runtime_error("类字段尚未初始化");
        }
        *result = &field;
    });
}

extern "C" int txrt_class_virtual_target(
    const void* value, std::size_t slot, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto& object = as_class(value);
        if (slot >= object.virtual_count)
        {
            throw std::runtime_error("虚方法槽索引越界");
        }
        if (object.virtual_targets[slot] == nullptr)
        {
            throw std::runtime_error("虚方法尚未实现");
        }
        *result = const_cast<void*>(object.virtual_targets[slot]);
    });
}

extern "C" int txrt_class_require_type(
    const void* value, const char* type_name) noexcept
{
    return invoke_checked([&]
    {
        const auto& item = *static_cast<const std::any*>(value);
        if (item.type() != typeid(tx_generated::class_handle))
        {
            throw std::runtime_error("类或接口转换失败：源值不是类对象");
        }
        const auto& object = as_class(value);
        if (std::find(object.ancestors.begin(), object.ancestors.end(),
                      type_name) == object.ancestors.end())
        {
            throw std::runtime_error("类或接口转换失败：对象不属于目标类型");
        }
    });
}
