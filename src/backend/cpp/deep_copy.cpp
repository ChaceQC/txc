#include "backend/cpp/value_abi.hpp"

#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/value_format.hpp"
#include "backend/cpp/vector_value.hpp"
#include "backend/cpp/container_value.hpp"
#include "stdlib/iterator.hpp"
#include "stdlib/typed_deque.hpp"
#include "stdlib/closure.hpp"
#include "stdlib/cancellation.hpp"
#include "stdlib/stdlib.hpp"
#include "stdlib/bytes.hpp"
#include "stdlib/file_stream.hpp"
#include "stdlib/encoding_incremental.hpp"
#include "stdlib/regex.hpp"
#include "stdlib/filesystem_watch.hpp"
#include "stdlib/process.hpp"
#include "stdlib/json_stream.hpp"
#include "stdlib/cbor.hpp"
#include "stdlib/csv.hpp"
#include "stdlib/xml.hpp"

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
        if (const auto* container = std::any_cast<container_handle>(&value))
        {
            const auto found = copies_.find(container->get());
            if (found != copies_.end())
            {
                return found->second;
            }
            if (const auto deque = std::dynamic_pointer_cast<object_deque_storage>(*container))
            {
                return copy_object_deque(*container, *deque);
            }
            if (const auto* graph = dynamic_cast<const graph_copyable*>(container->get()))
            {
                auto result = graph->empty_graph_copy();
                copies_.emplace(container->get(), result);
                graph->fill_graph_copy(*result, [&](const std::any& item)
                {
                    return copy(item);
                });
                return result;
            }
            std::any result = (*container)->copy();
            copies_.emplace(container->get(), result);
            return result;
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
        if (value.type() == typeid(byte_value))
        {
            // bytes 载荷不可变，深拷贝仍可安全共享。
            return value;
        }
        if (value.type() == typeid(cancel_source) ||
            value.type() == typeid(cancel_token) ||
            value.type() == typeid(regex_pattern))
        {
            // 取消令牌复制共享同一状态，不能复制成独立取消域。
            return value;
        }
        if (value.type() == typeid(binary_stream) ||
            value.type() == typeid(text_stream) ||
            value.type() == typeid(encoding_decoder) ||
            value.type() == typeid(encoding_encoder))
        {
            throw std::runtime_error("deep_copy 不支持复制文件流或增量编解码状态");
        }
        if (value.type() == typeid(process_child) || value.type() == typeid(process_pipe))
        {
            throw std::runtime_error("deep_copy 不支持复制子进程或管道");
        }
        if (value.type() == typeid(json_reader) || value.type() == typeid(json_writer))
        {
            throw std::runtime_error("deep_copy 不支持复制 JSON 游标");
        }
        if (value.type() == typeid(cbor_reader) || value.type() == typeid(cbor_writer))
        {
            throw std::runtime_error("deep_copy 不支持复制 CBOR 游标");
        }
        if (value.type() == typeid(csv_reader) || value.type() == typeid(csv_writer))
        {
            throw std::runtime_error("deep_copy 不支持复制 CSV 游标");
        }
        if (value.type() == typeid(xml_reader) || value.type() == typeid(xml_writer) ||
            value.type() == typeid(xml_document) || value.type() == typeid(xml_node))
        {
            throw std::runtime_error("deep_copy 不支持复制 XML 句柄");
        }
        if (value.type() == typeid(fs_watcher))
        {
            throw std::runtime_error("deep_copy 不支持复制文件监视器");
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
        tx_iterator result(iterator_state{{}, state.element_type, state.index,
            state.exhausted, state.closed});
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
        dynamic_struct result(dynamic_struct_data{
            source->type_name, source->display_name,
            struct_fields(source->fields.size()), source->metadata_owner});
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
        object->owned_ancestors = source->owned_ancestors;
        object->ancestors = object->owned_ancestors.empty()
            ? source->ancestors : object->owned_ancestors.data();
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
