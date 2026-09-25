#include "backend/cpp/value_abi.hpp"

#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/value_format.hpp"
#include "backend/cpp/vector_value.hpp"
#include "stdlib/stdlib.hpp"

#include <any>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace tx_generated
{
namespace
{

class copy_context
{
public:
    [[nodiscard]] std::any copy(const std::any& value)
    {
        std::any vector_result;
        if (visit_vector(value, [&](const auto& vector)
        {
            const auto found = copies_.find(vector.identity());
            if (found != copies_.end())
            {
                vector_result = found->second;
            }
            else
            {
                vector_result = vector.copy();
                copies_.emplace(vector.identity(), vector_result);
            }
        }))
        {
            return vector_result;
        }
        if (value.type() == typeid(tx_array))
        {
            return copy_array(std::any_cast<const tx_array&>(value));
        }
        if (value.type() == typeid(tx_dict))
        {
            return copy_dict(std::any_cast<const tx_dict&>(value));
        }
        if (value.type() == typeid(dynamic_struct))
        {
            return copy_struct(std::any_cast<const dynamic_struct&>(value));
        }
        if (value.type() == typeid(class_handle))
        {
            return copy_class(std::any_cast<const class_handle&>(value));
        }
        if (!value.has_value() || value.type() == typeid(std::int64_t) ||
            value.type() == typeid(double) || value.type() == typeid(bool) ||
            value.type() == typeid(std::string))
        {
            return value;
        }
        throw std::runtime_error("deep_copy 不支持此运行时类型");
    }

    void finish() noexcept
    {
        for (const auto& object : classes_)
        {
            object->destroying = false;
        }
    }

private:
    [[nodiscard]] std::any copy_array(const tx_array& source)
    {
        if (const auto found = copies_.find(source.identity()); found != copies_.end())
        {
            return found->second;
        }
        tx_array result;
        copies_.emplace(source.identity(), result);
        result.reserve(source.size());
        for (const auto& item : source)
        {
            result.push_back(copy(item));
        }
        return result;
    }

    [[nodiscard]] std::any copy_dict(const tx_dict& source)
    {
        if (const auto found = copies_.find(source.identity()); found != copies_.end())
        {
            return found->second;
        }
        tx_dict result;
        copies_.emplace(source.identity(), result);
        source.for_each([&](const std::any& key, const std::any& item)
        {
            result.emplace_back(copy(key), copy(item));
        });
        return result;
    }

    [[nodiscard]] std::any copy_struct(const dynamic_struct& source)
    {
        if (const auto found = copies_.find(source.identity()); found != copies_.end())
        {
            return found->second;
        }
        dynamic_struct result(dynamic_struct_data{
            source->type_name, source->display_name,
            struct_fields(source->fields.size())});
        copies_.emplace(source.identity(), result);
        for (std::size_t index = 0; index < source->fields.size(); ++index)
        {
            result->fields[index] = {
                source->fields[index].name, copy(source->fields[index].value)};
        }
        return result;
    }

    [[nodiscard]] std::any copy_class(const class_handle& source)
    {
        if (!source)
        {
            return source;
        }
        if (const auto found = copies_.find(source.operator->());
            found != copies_.end())
        {
            return found->second;
        }
        auto object = std::make_shared<dynamic_class>();
        object->type_name = source->type_name;
        object->display_name = source->display_name;
        object->ancestors = source->ancestors;
        object->ancestor_count = source->ancestor_count;
        object->virtual_targets = source->virtual_targets;
        object->virtual_count = source->virtual_count;
        object->destructor_targets = source->destructor_targets;
        object->destructor_count = source->destructor_count;
        object->fields.resize(source->fields.size());
        // 未完成的副本不应在复制失败时执行 deinit。
        object->destroying = true;
        register_class_gc(object);
        class_handle result(object);
        copies_.emplace(source.operator->(), result);
        classes_.push_back(std::move(object));
        for (std::size_t index = 0; index < source->fields.size(); ++index)
        {
            result->fields[index] = copy(source->fields[index]);
        }
        return result;
    }

    std::unordered_map<const void*, std::any> copies_;
    std::vector<std::shared_ptr<dynamic_class>> classes_;
};

} // namespace

std::any deep_copy_value(const std::any& value)
{
    copy_context context;
    auto result = context.copy(value);
    context.finish();
    return result;
}

} // namespace tx_generated

extern "C" int txrt_value_deep_copy(const void* value, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::detail::make_handle<std::any>(tx_generated::deep_copy_value(
            *static_cast<const std::any*>(value)));
    });
}
