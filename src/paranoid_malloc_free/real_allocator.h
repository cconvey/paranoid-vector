#pragma once

#include <cstdlib>
#include "real_heap_funcs.h"

template <class T>
struct real_allocator {
      real_allocator() = default;

      using value_type = T;

      template <class U> real_allocator(const real_allocator<U>& rhs) noexcept {}

      value_type* allocate (std::size_t n) {
          return static_cast<value_type*>(real_malloc(n*sizeof(value_type)));
      }

      void deallocate (T* p, std::size_t n) {
          real_free(p);
      }
};

template <class T, class U>
constexpr bool operator== (const real_allocator<T>& t, const real_allocator<U>& u) noexcept
{
    return true;
}

template <class T, class U>
constexpr bool operator!= (const real_allocator<T>& t, const real_allocator<U>& u) noexcept
{
    return !(t == u);
}
