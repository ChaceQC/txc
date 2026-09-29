#include "backend/cpp/typed_slots.hpp"

#include <stdexcept>

namespace tx_generated
{

typed_slots::typed_slots(std::span<const slot_kind> kinds, bool borrow_kinds)
    : owned_kinds_(borrow_kinds ? std::vector<slot_kind>{} :
                   std::vector<slot_kind>(kinds.begin(), kinds.end())),
      kinds_(borrow_kinds ? kinds : std::span<const slot_kind>(owned_kinds_)),
      slots_(kinds.size() > local_.size() ? kinds.size() : 0)
{
    for (std::size_t index = 0; index < kinds.size(); ++index)
    {
        if (kinds[index] == slot_kind::reference)
        {
            references_.push_back(std::make_unique<std::any>());
            data()[index].reference = references_.back().get();
        }
        else if (kinds[index] == slot_kind::floating)
        {
            data()[index].floating = 0.0;
        }
        else if (kinds[index] == slot_kind::boolean)
        {
            data()[index].boolean = false;
        }
    }
}

std::size_t typed_slots::size() const noexcept
{
    return kinds_.size();
}

typed_slot* typed_slots::data() noexcept
{
    return kinds_.size() <= local_.size() ? local_.data() : slots_.data();
}

const typed_slot* typed_slots::data() const noexcept
{
    return kinds_.size() <= local_.size() ? local_.data() : slots_.data();
}

slot_kind typed_slots::kind(std::size_t index) const
{
    if (index >= kinds_.size())
    {
        throw std::out_of_range("静态字段索引越界");
    }
    return kinds_[index];
}

std::any typed_slots::read(std::size_t index) const
{
    switch (kind(index))
    {
    case slot_kind::integer:
        return data()[index].integer;
    case slot_kind::floating:
        return data()[index].floating;
    case slot_kind::boolean:
        return data()[index].boolean;
    default:
        return *data()[index].reference;
    }
}

std::any& typed_slots::reference(std::size_t index) const
{
    if (kind(index) != slot_kind::reference)
    {
        throw std::runtime_error("标量槽不能作为动态引用访问");
    }
    return *data()[index].reference;
}

void typed_slots::write(std::size_t index, std::any value)
{
    switch (kind(index))
    {
    case slot_kind::integer:
        data()[index].integer = std::any_cast<std::int64_t>(value);
        break;
    case slot_kind::floating:
        data()[index].floating = std::any_cast<double>(value);
        break;
    case slot_kind::boolean:
        data()[index].boolean = std::any_cast<bool>(value);
        break;
    default:
        *data()[index].reference = std::move(value);
        break;
    }
}

void typed_slots::clear() noexcept
{
    for (auto& reference : references_)
    {
        reference->reset();
    }
}

} // namespace tx_generated
