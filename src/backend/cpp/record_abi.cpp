#include "backend/cpp/value_format.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/runtime_abi.hpp"

#include <stdexcept>

namespace
{

tx_generated::record_view& view_of(const void* value)
{
    const auto& input = *static_cast<const std::any*>(value);
    if (const auto* object = std::any_cast<tx_generated::dynamic_struct>(&input))
    {
        return (*object)->view;
    }
    if (const auto* object = std::any_cast<tx_generated::class_handle>(&input);
        object && *object)
    {
        return (*object)->view;
    }
    throw std::runtime_error("静态字段访问需要结构体或类对象");
}

} // namespace

extern "C" tx_generated::record_view* txrt_record_view(const void* value) noexcept
{
    tx_generated::record_view* result = nullptr;
    txrt_require_success(tx_generated::detail::invoke_leaf([&]
    {
        result = &view_of(value);
        if (!result->type)
        {
            throw std::runtime_error("对象布局与静态类型描述不一致");
        }
    }));
    return result;
}

extern "C" tx_generated::record_view* txrt_record_struct_view(const void* value) noexcept
{
    const auto* object = std::any_cast<tx_generated::dynamic_struct>(
        static_cast<const std::any*>(value));
    if (object && (*object)->view.type)
    {
        return &(*object)->view;
    }
    return txrt_record_view(value);
}

extern "C" tx_generated::record_view* txrt_record_class_view(const void* value) noexcept
{
    const auto* object = std::any_cast<tx_generated::class_handle>(
        static_cast<const std::any*>(value));
    if (object && *object && (*object)->view.type)
    {
        return &(*object)->view;
    }
    return txrt_record_view(value);
}

extern "C" int txrt_record_struct_new(const tx_generated::record_type* type,
    void** result) noexcept
{
    return tx_generated::detail::invoke_leaf([&]
    {
        *result = tx_generated::detail::make_handle<std::any>(
            tx_generated::dynamic_struct(tx_generated::dynamic_struct_data(type)));
    });
}

extern "C" int txrt_struct_field_get(const void* value, const char* name, void** result) noexcept
{
    return tx_generated::detail::invoke_leaf([&]
    {
        const auto* object = std::any_cast<tx_generated::dynamic_struct>(
            static_cast<const std::any*>(value));
        if (!object)
        {
            throw std::runtime_error("字段访问需要结构体类型");
        }
        for (std::size_t index = 0; index < (*object)->field_count(); ++index)
        {
            const auto* field = (*object)->field_name(index);
            if (field && std::string_view(field) == name)
            {
                *result = tx_generated::detail::make_handle<std::any>((*object)->read_field(index));
                return;
            }
        }
        throw std::runtime_error("字段不存在或对象不是结构体");
    });
}

extern "C" void txrt_record_require_type(const void* value,
    const tx_generated::record_type* type) noexcept
{
    txrt_require_success(tx_generated::detail::invoke_leaf([&]
    {
        const auto* object = std::any_cast<tx_generated::class_handle>(
            static_cast<const std::any*>(value));
        if (!object)
        {
            throw std::runtime_error("类或接口转换失败：源值不是类对象");
        }
        if (!*object)
        {
            throw std::runtime_error("class 对象引用为空");
        }
        const auto& actual = view_of(value);
        if (actual.type == type)
        {
            return;
        }
        if (actual.type)
        {
            for (std::size_t index = 0; index < actual.type->ancestor_count; ++index)
            {
                if (actual.type->ancestors[index] == type)
                {
                    return;
                }
            }
        }
        throw std::runtime_error("类或接口转换失败：对象不属于目标类型");
    }));
}

extern "C" void txrt_record_require_initialized(const void* field) noexcept
{
    txrt_require_success(tx_generated::detail::invoke_leaf([&]
    {
        if (!static_cast<const std::any*>(field)->has_value())
        {
            throw std::runtime_error("类字段尚未初始化");
        }
    }));
}
