#pragma once

#include "paranoia_pool.h"
#include "paranoia_allocator.h"

/// This class tries to provide the same post-instantiation interface as std::vector<T>,
/// but using a paranoia_allocator instead of std::allocator.
///
/// The class' interface is not currently complete; we add to it as needed.
template <typename T>
class paranoid_vector {
    public:
        using value_type             = T;
        using allocator_type         = paranoia_allocator<T>;
        using size_type              = std::size_t;
        using difference_type        = std::ptrdiff_t;
        using reference              = T&;
        using const_reference        = const T&;
        using pointer                = std::allocator_traits<allocator_type>::pointer;
        using const_pointer          = std::allocator_traits<allocator_type>::const_pointer;
        using iterator               = pointer;
        using const_iterator         = const_pointer;
        using reverse_iterator       = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        paranoid_vector();
        explicit paranoid_vector(const allocator_type& alloc);

        ~paranoid_vector();

        size_type size() const {
            return current_num_elems_;
        }


    private:
        size_t capacity_num_elems_;
        size_t current_num_elems_;


};

