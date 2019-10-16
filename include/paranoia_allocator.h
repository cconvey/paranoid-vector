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
#if PARANOIA_LOGGING
          std::cout << __PRETTY_FUNCTION__ << " (" << __LINE__ << ")" << " :"
              << " this=" << std::hex << std::showbase << this << std::dec
              << std::endl;
#endif
      }

      paranoia_allocator(std::shared_ptr<ParanoiaPool> ppool = g_paranoia_default_pool) noexcept
          : ppool_(ppool)
      {
#if PARANOIA_LOGGING
          std::cout << __PRETTY_FUNCTION__ << " (" << __LINE__ << ")" << " :"
              << " this=" << std::hex << std::showbase << this << std::dec
              << std::endl;
#endif
      }

      template <class U>
          paranoia_allocator(const paranoia_allocator<U>& rhs) noexcept
      : ppool_(rhs.ppool_)
      {
#if PARANOIA_LOGGING
          std::cout << __PRETTY_FUNCTION__ << " (" << __LINE__ << ")" << " :"
              << " this=" << std::hex << std::showbase << this << std::dec
              << std::endl;
#endif
      }

      ~paranoia_allocator() noexcept {
#if PARANOIA_LOGGING
          std::cout << __PRETTY_FUNCTION__ << " (" << __LINE__ << ")" << " :"
              << " this=" << std::hex << std::showbase << this << std::dec
              << std::endl;
#endif
      }

      T* allocate (std::size_t n) {
#if PARANOIA_LOGGING
          std::cout << __PRETTY_FUNCTION__ << " (" << __LINE__ << ")" << " :"
            << " this=" << std::hex << std::showbase << this << std::dec
            << " n=" << n
            << std::endl;
#endif

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
#if PARANOIA_LOGGING
          std::cout << __PRETTY_FUNCTION__ << " (" << __LINE__ << ")" << " :"
            << " this=" << std::hex << std::showbase << this << std::dec
            << " n=" << n
            << " p=" << std::hex << std::showbase << p << std::dec
            << std::endl;
#endif
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
#if PARANOIA_LOGGING
    std::cout << __PRETTY_FUNCTION__ << " :"
        << " t=" << std::hex << std::showbase << t << std::dec
        << " u=" << std::hex << std::showbase << u << std::dec
        << std::endl;
#endif
    return (&t) == (&u);
}

template <class T, class U>
constexpr bool operator!= (const paranoia_allocator<T>& t, const paranoia_allocator<U>& u) noexcept
{
    return !(t == u);
}
