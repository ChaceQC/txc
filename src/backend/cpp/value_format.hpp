#pragma once

#include "backend/cpp/cycle_gc.hpp"

#include <any>
#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace tx_generated
{

struct dynamic_field
{
    const char* name = nullptr;
    std::any value;
};

class struct_fields
{
public:
    explicit struct_fields(std::size_t count = 0) : count_(count)
    {
        if (count > inline_count)
        {
            overflow_.resize(count);
        }
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return count_;
    }
    [[nodiscard]] dynamic_field& operator[](std::size_t index) noexcept
    {
        return data()[index];
    }
    [[nodiscard]] const dynamic_field& operator[](std::size_t index) const noexcept
    {
        return data()[index];
    }
    [[nodiscard]] dynamic_field* begin() noexcept
    {
        return data();
    }
    [[nodiscard]] dynamic_field* end() noexcept
    {
        return data() + count_;
    }
    [[nodiscard]] const dynamic_field* begin() const noexcept
    {
        return data();
    }
    [[nodiscard]] const dynamic_field* end() const noexcept
    {
        return data() + count_;
    }
    void clear() noexcept
    {
        if (count_ > inline_count)
        {
            overflow_.clear();
        }
        else
        {
            for (std::size_t index = 0; index < count_; ++index)
            {
                local_[index].value.reset();
                local_[index].name = nullptr;
            }
        }
        count_ = 0;
    }

private:
    static constexpr std::size_t inline_count = 2;
    [[nodiscard]] dynamic_field* data() noexcept
    {
        return count_ > inline_count ? overflow_.data() : local_.data();
    }
    [[nodiscard]] const dynamic_field* data() const noexcept
    {
        return count_ > inline_count ? overflow_.data() : local_.data();
    }

    std::array<dynamic_field, inline_count> local_{};
    std::vector<dynamic_field> overflow_;
    std::size_t count_ = 0;
};

struct dynamic_struct_data
{
    std::string type_name;
    std::string display_name;
    struct_fields fields;
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
    const char* type_name = nullptr;
    const char* display_name = nullptr;
    const char* const* ancestors = nullptr;
    std::size_t ancestor_count = 0;
    std::vector<const char*> owned_ancestors;
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
