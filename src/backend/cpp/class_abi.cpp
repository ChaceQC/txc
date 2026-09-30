#include "backend/cpp/value_abi.hpp"

#include "backend/cpp/runtime.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/runtime_abi.hpp"
#include "backend/cpp/value_format.hpp"
#include "stdlib/vector.hpp"
#include "stdlib/bytes.hpp"
#include "stdlib/file_stream.hpp"

#include <any>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
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
    if (object->destructor_count == 0)
    {
        return true;
    }
    std::any receiver = class_handle(object);
    for (std::size_t index = 0; index < object->destructor_count; ++index)
    {
        // 析构回调借用栈上接收者；额外引用保证回调期间字段仍然有效。
        detail::error_cleanup_guard error_guard;
        if (object->fixed.type && object->fixed.type->view_destructors)
        {
            using destructor_view_fn = void (*)(detail::runtime_context*, void*, record_view*);
            auto callback = reinterpret_cast<destructor_view_fn>(
                const_cast<void*>(object->fixed.type->view_destructors[index]));
            callback(&detail::current_runtime_context(), &receiver, &object->view);
            continue;
        }
        using destructor_fn = void (*)(detail::runtime_context*, void*);
        auto callback = reinterpret_cast<destructor_fn>(
            const_cast<void*>(object->destructor_targets[index]));
        callback(&detail::current_runtime_context(), &receiver);
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
    // 没有向外对象边就不能形成环；最后释放仍由 class_handle 执行析构和复活。
    if (object->fixed.type && object->fixed.type->scan_count == 0)
    {
        return;
    }
    register_gc_node(object,
        [](const void* value, gc_visit visit, void* context)
        {
            const auto& data = *static_cast<const dynamic_class*>(value);
            if (data.fixed.type)
            {
                data.fixed.scan(visit, context);
                return;
            }
            for (const auto& field : static_cast<const dynamic_class*>(value)->fields)
            {
                visit(field, context);
            }
        },
        [](void* value)
        {
            static_cast<dynamic_class*>(value)->fields.clear();
            static_cast<dynamic_class*>(value)->fixed.slots.clear();
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

} // namespace

namespace tx_generated
{

std::any class_default_field(const std::string& type)
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
    if (type == "bytes")
    {
        return tx_generated::make_bytes({});
    }
    if (type == "binary_stream")
    {
        return tx_generated::binary_stream{};
    }
    if (type == "text_stream")
    {
        return tx_generated::text_stream{};
    }
    if (type == "array")
    {
        return tx_generated::tx_array{};
    }
    if (type == "dict")
    {
        return tx_generated::tx_dict{};
    }
    if (type == "vector<int>")
    {
        return tx_generated::int_vector{};
    }
    if (type == "vector<float>")
    {
        return tx_generated::float_vector{};
    }
    if (type == "vector<bool>")
    {
        return tx_generated::bool_vector{};
    }
    if (type == "vector<str>")
    {
        return tx_generated::string_vector{};
    }
    if (type == "vector<bytes>")
    {
        return tx_generated::bytes_vector{};
    }
    if (type.starts_with("vector<") && type.ends_with(">"))
    {
        return tx_generated::object_vector(
            type.substr(7, type.size() - 8));
    }
    return {};
}

} // namespace tx_generated

namespace
{

template<class field_type>
void* scalar_field_ptr(const void* value, std::size_t index,
                       const char* type_name) noexcept
{
    const auto* item = static_cast<const std::any*>(value);
    const auto* handle = item
        ? std::any_cast<tx_generated::class_handle>(item) : nullptr;
    if (handle && *handle && (*handle)->fixed.type &&
        index < (*handle)->fixed.slots.size() &&
        (*handle)->fixed.slots.kind(index) == (std::is_same_v<field_type, std::int64_t>
            ? tx_generated::slot_kind::integer : std::is_same_v<field_type, double>
            ? tx_generated::slot_kind::floating : tx_generated::slot_kind::boolean))
    {
        return (*handle)->fixed.slots.data() + index;
    }
    if (handle && *handle && index < (*handle)->fields.size())
    {
        auto& field = (*handle)->fields[index];
        if (auto* result = std::any_cast<field_type>(&field))
        {
            return result;
        }
    }
    std::snprintf(tx_generated::detail::current_runtime_context().last_error, 256,
                  "类字段不是 %s 或索引无效", type_name);
    txrt_require_success(1);
    return nullptr;
}

} // namespace

using tx_generated::detail::invoke_checked;

extern "C" int txrt_record_class_new(const tx_generated::record_type* type,
    void** result) noexcept
{
    return invoke_checked([&]
    {
        auto object = std::make_shared<tx_generated::dynamic_class>();
        object->type_name = type->name;
        object->display_name = type->display_name;
        object->ancestors = type->ancestor_names;
        object->ancestor_count = type->ancestor_count;
        object->virtual_targets = type->virtual_targets;
        object->virtual_count = type->virtual_count;
        object->destructor_targets = type->destructors;
        object->destructor_count = type->destructor_count;
        object->fixed = tx_generated::record_storage(type);
        object->view = {object->fixed.slots.data(), type};
        for (std::size_t index = 0; index < type->field_count; ++index)
        {
            if (type->fields[index].kind == 0 && type->fields[index].type_name)
            {
                object->fixed.slots.reference(index) = tx_generated::class_default_field(type->fields[index].type_name);
            }
        }
        tx_generated::register_class_gc(object);
        *result = tx_generated::detail::make_handle<std::any>(
            tx_generated::class_handle(std::move(object)));
    });
}

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
        object->ancestors = ancestors;
        object->ancestor_count = ancestor_count;
        object->fields.reserve(field_count);
        for (std::size_t i = 0; i < field_count; ++i)
        {
            object->fields.push_back(field_types[i] == nullptr
                ? std::any{} : tx_generated::class_default_field(field_types[i]));
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
        if (object.fixed.type)
        {
            auto& field = object.fixed.slots.reference(index);
            if (!for_write && !field.has_value())
            {
                throw std::runtime_error("类字段尚未初始化");
            }
            *result = &field;
            return;
        }
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

extern "C" void* txrt_class_field_i64_ptr(const void* value,
                                            std::size_t index) noexcept
{
    return scalar_field_ptr<std::int64_t>(value, index, "int");
}

extern "C" void* txrt_class_field_f64_ptr(const void* value,
                                            std::size_t index) noexcept
{
    return scalar_field_ptr<double>(value, index, "float");
}

extern "C" void* txrt_class_field_bool_ptr(const void* value,
                                             std::size_t index) noexcept
{
    return scalar_field_ptr<bool>(value, index, "bool");
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

extern "C" void* txrt_class_virtual_target_fast(
    const void* value, std::size_t slot) noexcept
{
    const auto* item = static_cast<const std::any*>(value);
    const auto* handle = item
        ? std::any_cast<tx_generated::class_handle>(item) : nullptr;
    if (handle && *handle && slot < (*handle)->virtual_count &&
        (*handle)->virtual_targets[slot])
    {
        return const_cast<void*>((*handle)->virtual_targets[slot]);
    }
    std::snprintf(tx_generated::detail::current_runtime_context().last_error, 256,
                  "虚方法槽索引越界或尚未实现");
    txrt_require_success(1);
    return nullptr;
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
        bool matches = false;
        for (std::size_t index = 0; index < object.ancestor_count; ++index)
        {
            matches |= std::string_view(object.ancestors[index]) == type_name;
        }
        if (!matches)
        {
            throw std::runtime_error("类或接口转换失败：对象不属于目标类型");
        }
    });
}

extern "C" void txrt_class_require_type_fast(
    const void* value, const char* type_name) noexcept
{
    const auto* item = static_cast<const std::any*>(value);
    const auto* handle = item
        ? std::any_cast<tx_generated::class_handle>(item) : nullptr;
    if (handle && *handle)
    {
        for (std::size_t index = 0; index < (*handle)->ancestor_count; ++index)
        {
            if (std::string_view((*handle)->ancestors[index]) == type_name)
            {
                return;
            }
        }
    }
    txrt_require_success(txrt_class_require_type(value, type_name));
}
