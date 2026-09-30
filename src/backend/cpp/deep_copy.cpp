#include "backend/cpp/value_abi.hpp"
#include "backend/cpp/deep_copy.hpp"
#include "backend/cpp/identity_table.hpp"

#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/value_format.hpp"
#include "backend/cpp/vector_value.hpp"
#include "backend/cpp/container_value.hpp"
#include "stdlib/iterator.hpp"
#include "stdlib/typed_deque.hpp"
#include "stdlib/closure.hpp"
#include "stdlib/stdlib.hpp"
#include "stdlib/bytes.hpp"

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
        // 纯值占数组/字典图中的大多数叶子，避免依次试探容器和外部资源。
        if (!value.has_value() || value.type() == typeid(std::int64_t) ||
            value.type() == typeid(double) || value.type() == typeid(bool) ||
            value.type() == typeid(std::string) || value.type() == typeid(byte_value))
        {
            return value;
        }
        // 通用对象图的数组和字典优先处理，不先试探全部向量实例化类型。
        if (const auto* array = std::any_cast<tx_array>(&value))
        {
            return copy_array(*array);
        }
        if (const auto* dictionary = std::any_cast<tx_dict>(&value))
        {
            return copy_dict(*dictionary);
        }
        if (const auto* container = std::any_cast<container_handle>(&value))
        {
            return copy_container(*container);
        }
        if (const auto* iterator = std::any_cast<tx_iterator>(&value))
        {
            return copy_iterator(*iterator);
        }
        if (const auto* closure = std::any_cast<closure_handle>(&value))
        {
            return copy_closure(*closure);
        }
        if (const auto* vector = std::any_cast<object_vector>(&value))
        {
            return copy_object_vector(*vector);
        }
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
        if (value.type() == typeid(dynamic_struct))
        {
            return copy_struct(std::any_cast<const dynamic_struct&>(value));
        }
        if (value.type() == typeid(class_handle))
        {
            return copy_class(std::any_cast<const class_handle&>(value));
        }
        return copy_resource_value(value);
    }

    void finish() noexcept
    {
        for (const auto& object : classes_)
        {
            object->destroying = false;
        }
    }

private:
    [[nodiscard]] std::any copy_container(const container_handle& source)
    {
        const auto found = copies_.find(source.get());
        if (found != copies_.end())
        {
            return found->second;
        }
        if (const auto deque = std::dynamic_pointer_cast<object_deque_storage>(source))
        {
            return copy_object_deque(source, *deque);
        }
        if (const auto* graph = dynamic_cast<const graph_copyable*>(source.get()))
        {
            auto result = graph->empty_graph_copy();
            copies_.emplace(source.get(), result);
            graph->fill_graph_copy(*result, [&](const std::any& item)
            {
                return copy(item);
            });
            return result;
        }
        std::any result = source->copy();
        copies_.emplace(source.get(), result);
        return result;
    }

    [[nodiscard]] std::any copy_object_deque(
        const container_handle& source, const object_deque_storage& values)
    {
        auto storage = std::make_shared<object_deque_storage>(values.element_name);
        register_object_deque(storage);
        container_handle result = storage;
        copies_.emplace(source.get(), result);
        for (const auto& item : values.values)
        {
            storage->values.push_back(copy(item));
        }
        return result;
    }

    [[nodiscard]] std::any copy_closure(const closure_handle& source)
    {
        if (const auto found = copies_.find(source.identity());
            found != copies_.end())
        {
            return found->second;
        }
        const auto& state = source.data();
        closure_handle result(closure_state{
            state.target, state.type_name, {}, {}});
        copies_.emplace(source.identity(), result);
        result.data().parent = copy(state.parent);
        result.data().captures.reserve(state.captures.size());
        for (const auto& capture : state.captures)
        {
            result.data().captures.push_back(copy(capture));
        }
        std::vector<slot_kind> kinds;
        for (std::size_t index = 0; index < state.typed_captures.size(); ++index)
        {
            kinds.push_back(state.typed_captures.kind(index));
        }
        result.data().typed_captures = typed_slots(kinds);
        copy_slots(state.typed_captures, result.data().typed_captures);
        result.data().view.code = state.view.code;
        result.data().refresh();
        return result;
    }

    [[nodiscard]] std::any copy_iterator(const tx_iterator& source)
    {
        if (const auto found = copies_.find(source.identity());
            found != copies_.end())
        {
            return found->second;
        }
        const auto& state = source.data();
        tx_iterator result(iterator_state{{}, state.element_type,
            {nullptr, 0, state.cursor.index, state.cursor.exhausted, state.cursor.closed}});
        copies_.emplace(source.identity(), result);
        result.data().values = copy(state.values);
        return result;
    }

    [[nodiscard]] std::any copy_object_vector(const object_vector& source)
    {
        if (const auto found = copies_.find(source.identity()); found != copies_.end())
        {
            return found->second;
        }
        object_vector result(source.data().type_name);
        copies_.emplace(source.identity(), result);
        auto& values = result.data().values;
        values.reserve(source.data().values.size());
        for (const auto& item : source.data().values)
        {
            values.push_back(copy(item));
        }
        result.data().refresh();
        return result;
    }

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
        dynamic_struct result(source->fixed.type ? dynamic_struct_data(source->fixed.type) :
            dynamic_struct_data{source->type_name, source->display_name,
                struct_fields(source->fields.size()), source->metadata_owner});
        copies_.emplace(source.identity(), result);
        if (source->fixed.type)
        {
            copy_slots(source->fixed.slots, result->fixed.slots, source->fixed.type);
            return result;
        }
        for (std::size_t index = 0; index < source->field_count(); ++index)
        {
            if (!source->fixed.type)
            {
                result->fields[index].name = source->fields[index].name;
            }
            result->write_field(index, copy(source->read_field(index)));
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
        object->owned_ancestors = source->owned_ancestors;
        object->ancestors = object->owned_ancestors.empty()
            ? source->ancestors : object->owned_ancestors.data();
        object->ancestor_count = source->ancestor_count;
        object->virtual_targets = source->virtual_targets;
        object->virtual_count = source->virtual_count;
        object->destructor_targets = source->destructor_targets;
        object->destructor_count = source->destructor_count;
        object->fields.resize(source->fields.size());
        if (source->fixed.type)
        {
            object->fixed = record_storage(source->fixed.type);
            object->view = {object->fixed.slots.data(), source->fixed.type};
        }
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
        copy_slots(source->fixed.slots, result->fixed.slots, source->fixed.type);
        return result;
    }

    template<class element>
    std::any copy_scalar_vector(const std::any& value)
    {
        const auto& source = std::any_cast<const tx_vector<element>&>(value);
        if (const auto found = copies_.find(source.identity()); found != copies_.end())
        {
            return found->second;
        }
        std::any result = source.copy();
        copies_.emplace(source.identity(), result);
        return result;
    }

    std::any copy_field(const std::any& value, tx::record_copy_kind kind)
    {
        // 未初始化引用保持 none；静态字段不再试探所有运行时类型。
        if (!value.has_value())
        {
            return {};
        }
        using copy_kind = tx::record_copy_kind;
        switch (kind)
        {
        case copy_kind::value: return value;
        case copy_kind::container: return copy_container(std::any_cast<const container_handle&>(value));
        case copy_kind::structure: return copy_struct(std::any_cast<const dynamic_struct&>(value));
        case copy_kind::class_object: return copy_class(std::any_cast<const class_handle&>(value));
        case copy_kind::array: return copy_array(std::any_cast<const tx_array&>(value));
        case copy_kind::dictionary: return copy_dict(std::any_cast<const tx_dict&>(value));
        case copy_kind::iterator: return copy_iterator(std::any_cast<const tx_iterator&>(value));
        case copy_kind::closure: return copy_closure(std::any_cast<const closure_handle&>(value));
        case copy_kind::vector_object: return copy_object_vector(std::any_cast<const object_vector&>(value));
        case copy_kind::vector_i64: return copy_scalar_vector<std::int64_t>(value);
        case copy_kind::vector_f64: return copy_scalar_vector<double>(value);
        case copy_kind::vector_bool: return copy_scalar_vector<std::uint8_t>(value);
        case copy_kind::vector_str: return copy_scalar_vector<text_reference>(value);
        case copy_kind::vector_bytes: return copy_scalar_vector<byte_value>(value);
        default: return copy(value);
        }
    }

    void copy_slots(const typed_slots& source, typed_slots& result,
        const record_type* type = nullptr)
    {
        if (type && type->copy_slots)
        {
            type->copy_slots(source.data(), result.data(),
                [](const std::any& value, std::any& destination, std::uint64_t kind, void* context)
                {
                    destination = static_cast<copy_context*>(context)->copy_field(
                        value, static_cast<tx::record_copy_kind>(kind));
                }, this);
            return;
        }
        for (std::size_t index = 0; index < source.size(); ++index)
        {
            if (source.kind(index) != slot_kind::reference)
            {
                result.data()[index] = source.data()[index];
            }
            else
            {
                result.reference(index) = copy_field(source.reference(index),
                    type ? type->fields[index].copy_kind : tx::record_copy_kind::unknown);
            }
        }
    }

    identity_table<std::any> copies_;
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
