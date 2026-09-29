#include "backend/cpp/typed_slots.hpp"
#include "backend/cpp/identity_table.hpp"
#include "stdlib/byte_storage.hpp"

#include <cassert>
#include <iostream>

int main()
{
    using namespace tx_generated;
    const slot_kind kinds[]{slot_kind::reference, slot_kind::integer, slot_kind::reference};
    typed_slots original(kinds);
    original.write(0, std::string("first"));
    original.write(1, std::int64_t{7});
    original.write(2, std::string("last"));
    typed_slots moved(std::move(original));
    assert(std::any_cast<std::string>(moved.reference(0)) == "first");
    assert(moved.data()[2].reference == &moved.reference(2));
    assert(std::any_cast<std::int64_t>(moved.read(1)) == 7);
    auto* stable = &moved.reference(0);
    moved.write(0, std::string("changed"));
    assert(stable == &moved.reference(0));
    identity_table<int> identities;
    int keys[40]{};
    for (int index = 0; index < 40; ++index)
    {
        identities.emplace(&keys[index], index);
    }
    for (int index = 0; index < 40; ++index)
    {
        assert(identities.find(&keys[index])->second == index);
    }
    byte_storage small(std::vector<std::uint8_t>{1, 2, 3});
    byte_storage large(std::vector<std::uint8_t>(64, 9));
    assert(small.size() == 3 && small[2] == 3 && large.size() == 64 && large[63] == 9);
    std::cout << "NATIVE_STORAGE_OK\n";
}
