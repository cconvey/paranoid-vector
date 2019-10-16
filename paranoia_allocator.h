#pragma once

#include <memory>
#include <iostream>
#include <iomanip>

#include "paranoia_pool.h"

template <class T>
struct paranoia_allocator {
      using value_type = T;

      std::shared_ptr<ParanoiaPool> ppool_ = nullptr;

      paranoia_allocator(const paranoia_allocator<T> & rhs) noexcept
      : ppool_(rhs.ppool_)
      {
          std::cout << __PRETTY_FUNCTION__ << " (" << __LINE__ << ")" << " :"
              << " this=" << std::hex << std::showbase << this << std::dec
              << std::endl;
      }

      // We don't need to define this ourselves, but it helps with debugging because
      // the it does get used.
      paranoia_allocator() noexcept
      {
          std::cout << __PRETTY_FUNCTION__ << " (" << __LINE__ << ")" << " :"
              << " this=" << std::hex << std::showbase << this << std::dec
              << std::endl;
      }

      template <class U>
          paranoia_allocator(const paranoia_allocator<U>& rhs) noexcept
      : ppool_(rhs.ppool_)
      {
          std::cout << __PRETTY_FUNCTION__ << " (" << __LINE__ << ")" << " :"
              << " this=" << std::hex << std::showbase << this << std::dec
              << std::endl;
      }

      ~paranoia_allocator() noexcept {
          std::cout << __PRETTY_FUNCTION__ << " (" << __LINE__ << ")" << " :"
              << " this=" << std::hex << std::showbase << this << std::dec
              << std::endl;
      }

      T* allocate (std::size_t n) {
          std::cout << __PRETTY_FUNCTION__ << " (" << __LINE__ << ")" << " :"
            << " this=" << std::hex << std::showbase << this << std::dec
            << " n=" << n
            << std::endl;

          void* p;
          if (ppool_) {
              // FIXME(cconvey): This should throw a std::bad_alloc for any error.
              p = ppool_->allocate(n*sizeof(value_type));
          }
          else {
              p = ::operator new (n*sizeof(value_type));
          };

          return static_cast<value_type*>(p);
      }

      void deallocate (T* p, std::size_t n) {
          std::cout << __PRETTY_FUNCTION__ << " (" << __LINE__ << ")" << " :"
            << " this=" << std::hex << std::showbase << this << std::dec
            << " n=" << n
            << " p=" << std::hex << std::showbase << p << std::dec
            << std::endl;
          if (ppool_) {
              ppool_->deallocate(p);
          }
          else {
              ::operator delete(p);
          }
      }
};

template <class T, class U>
constexpr bool operator== (const paranoia_allocator<T>& t, const paranoia_allocator<U>& u) noexcept
{
    std::cout << __PRETTY_FUNCTION__ << " :"
        << " t=" << std::hex << std::showbase << t << std::dec
        << " u=" << std::hex << std::showbase << u << std::dec
        << std::endl;
    return (&t) == (&u);
}

template <class T, class U>
constexpr bool operator!= (const paranoia_allocator<T>& t, const paranoia_allocator<U>& u) noexcept
{
    return !(t == u);
}
