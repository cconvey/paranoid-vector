#pragma once

#include <ostream>
#include <cassert>
#include <limits>

struct HexPtr {
    const void* const p;
    HexPtr(const void* p) : p(p) {}
};

std::ostream& operator<<(std::ostream& os, const HexPtr ap);

template <typename T>
T checked_cast(long val) {
    assert(val >= std::numeric_limits<T>::min());
    assert(val <= std::numeric_limits<T>::max());

    return T(val);
}

size_t get_page_size();

long get_vm_max_map_count();

