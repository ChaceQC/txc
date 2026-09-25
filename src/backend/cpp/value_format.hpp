#pragma once

#include "backend/cpp/cycle_gc.hpp"

#include <any>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace tx_generated
{

struct dynamic_field
{
    std::string name;
    std::any value;
};

struct dynamic_struct_data
{
    std::string type_name;
    std::string display_name;
    std::vector<dynamic_field> fields;
};

struct dynamic_struct
{
    explicit dynamic_struct(dynamic_struct_data value)
        : data(std::make_shared<dynamic_struct_data>(std::move(value)))
    {
        register_gc_node(data,
            [](const void* object, gc_visit visit, void* context)
            {
                for (const auto& field :
                     static_cast<const dynamic_struct_data*>(object)->fields)
                {
                    visit(field.value, context);
                }
            },
            [](void* object)
            {
                static_cast<dynamic_struct_data*>(object)->fields.clear();
            });
    }

    [[nodiscard]] dynamic_struct_data* operator->() const noexcept
    {
        return data.get();
    }
    [[nodiscard]] const void* identity() const noexcept
    {
        return data.get();
    }

    std::shared_ptr<dynamic_struct_data> data;
};

struct dynamic_class
{
    std::string type_name;
    std::string display_name;
    std::vector<std::string> ancestors;
    std::vector<std::any> fields;
    const void* const* virtual_targets = nullptr;
    std::size_t virtual_count = 0;
    const void* const* destructor_targets = nullptr;
    std::size_t destructor_count = 0;
    bool destroying = false;
};

class class_handle
{
public:
    explicit class_handle(std::shared_ptr<dynamic_class> object)
        : object_(std::move(object))
    {
    }
    class_handle(const class_handle&) = default;
    class_handle(class_handle&&) noexcept = default;
    class_handle& operator=(class_handle other) noexcept
    {
        object_.swap(other.object_);
        return *this;
    }
    ~class_handle();

    [[nodiscard]] dynamic_class& operator*() const noexcept
    {
        return *object_;
    }
    [[nodiscard]] dynamic_class* operator->() const noexcept
    {
        return object_.get();
    }
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return static_cast<bool>(object_);
    }

private:
    std::shared_ptr<dynamic_class> object_;
};

void register_class_gc(const std::shared_ptr<dynamic_class>& object);

[[nodiscard]] std::string format_print_value(const std::any& value);
[[nodiscard]] std::string format_repr_value(const std::any& value);

} // namespace tx_generated
