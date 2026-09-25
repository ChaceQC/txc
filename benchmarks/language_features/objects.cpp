#include "compare.hpp"
#include "workload.hpp"

#include <cstdint>
#include <chrono>
#include <memory>

namespace
{

struct point_data
{
    std::int64_t x;
    std::int64_t y;
};

class point_ref
{
public:
    explicit point_ref(point_data data)
        : value_(std::make_shared<point_data>(data))
    {
    }

    point_ref operator+(const point_ref& other) const
    {
        return point_ref(point_data{value_->x + other.value_->x,
                                    value_->y + other.value_->y});
    }

    point_ref& operator+=(const point_ref& other)
    {
        *this = *this + other;
        return *this;
    }

    bool operator==(const point_ref& other) const
    {
        return value_->x == other.value_->x &&
            value_->y == other.value_->y;
    }

    std::int64_t x() const
    {
        return value_->x;
    }

private:
    std::shared_ptr<point_data> value_;
};

class readable
{
public:
    virtual ~readable() = default;
    virtual std::int64_t read() const = 0;
};

class counter
{
public:
    explicit counter(std::int64_t value) : value_(value)
    {
    }

    virtual ~counter() = default;

    std::int64_t increase(std::int64_t delta)
    {
        value_ += delta;
        return value_;
    }

    std::int64_t increase(double delta)
    {
        value_ += static_cast<std::int64_t>(delta);
        return value_;
    }

    virtual std::int64_t read() const
    {
        return value_;
    }

    virtual std::int64_t operator+(const counter& other) const
    {
        return value_ + other.read();
    }

protected:
    std::int64_t value_;
};

class adjusted_counter : public counter, public readable
{
public:
    using counter::counter;

    std::int64_t read() const override
    {
        return counter::read() + 1;
    }

    std::int64_t operator+(const counter& other) const override
    {
        return counter::operator+(other) + 1;
    }
};

class alternate_counter : public counter, public readable
{
public:
    using counter::counter;

    std::int64_t read() const override
    {
        return value_ + 1;
    }

    std::int64_t operator+(const counter& other) const override
    {
        return counter::operator+(other) + 1;
    }
};

bool before_unix_epoch()
{
    return std::chrono::system_clock::now().time_since_epoch() <=
        std::chrono::system_clock::duration::zero();
}

} // namespace

void bench_struct_operators()
{
    std::int64_t checksum = 0;
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 100000; ++i)
    {
        point_ref current(point_data{.x = i, .y = 2});
        current += point_ref(point_data{3, 4});
        if (current == point_ref(point_data{i + 3, 6}))
        {
            checksum += current.x();
        }
    }
    report("struct_operators", started, checksum);
}

void bench_class_methods()
{
    counter value(0);
    std::int64_t checksum = 0;
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 200000; ++i)
    {
        checksum += value.increase(i / 1000 + 1);
        checksum += value.increase(1.0);
    }
    report("class_methods", started, checksum);
}

void bench_virtual_interface()
{
    std::shared_ptr<readable> interface_view =
        std::make_shared<adjusted_counter>(7);
    if (before_unix_epoch())
    {
        interface_view = std::make_shared<alternate_counter>(7);
    }
    std::shared_ptr<counter> parent_view =
        std::dynamic_pointer_cast<counter>(interface_view);
    std::int64_t checksum = 0;
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 200000; ++i)
    {
        checksum += interface_view->read() + parent_view->read();
    }
    report("virtual_interface", started, checksum);
}

void bench_class_operator()
{
    std::shared_ptr<counter> left = std::make_shared<adjusted_counter>(7);
    if (before_unix_epoch())
    {
        left = std::make_shared<alternate_counter>(7);
    }
    std::shared_ptr<counter> right = std::make_shared<counter>(2);
    std::int64_t checksum = 0;
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 100000; ++i)
    {
        checksum += *left + *right;
    }
    report("class_operator", started, checksum);
}

void bench_runtime_cast()
{
    std::shared_ptr<readable> interface_view =
        std::make_shared<adjusted_counter>(7);
    std::int64_t checksum = 0;
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 100000; ++i)
    {
        auto parent_view = std::dynamic_pointer_cast<counter>(interface_view);
        checksum += parent_view->read();
    }
    report("runtime_cast", started, checksum);
}

void bench_module_call()
{
    std::int64_t checksum = 0;
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 100000; ++i)
    {
        workload::pair item = std::make_shared<workload::pair_data>(
            workload::pair_data{i, i + 1});
        checksum += workload::bump(i) + workload::measure(item);
    }
    report("module_call", started, checksum);
}
