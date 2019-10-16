#pragma once

#include "util.h"
#include "paranoia_pool.h"
#include "paranoia_allocator.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <cassert>
#include <initializer_list>
#include <iterator>

// LIMITATIONS:
// - Not all vector methods / members are provided.
// - Does not guarantee alignment requirements of stored elements.
// - Does not leverage std::allocator_traits as much as it probably should.

// WISHLIST:
// - Make this thead-aware.to look for illegal concurrent use.

// TODO: Make this class thread-aware, by using a test-and-set bit to look for illegal concurrency.

template <typename T>
class paranoid_vector {
    public:
        using value_type             = T;
        using allocator_type         = paranoia_allocator<T>;
        using size_type              = std::size_t;
        using difference_type        = std::ptrdiff_t;
        using reference              = T&;
        using const_reference        = const T&;
        using pointer                = typename std::allocator_traits<allocator_type>::pointer;
        using const_pointer          = typename std::allocator_traits<allocator_type>::const_pointer;
        using iterator               = pointer;
        using const_iterator         = const_pointer;
        using reverse_iterator       = typename std::reverse_iterator<iterator>;
        using const_reverse_iterator = typename std::reverse_iterator<const_iterator>;

        explicit paranoid_vector(size_t count);
        paranoid_vector(std::shared_ptr<allocator_type> allocator = std::make_shared<allocator_type>());
        paranoid_vector(const paranoid_vector<T>& other);
        paranoid_vector(std::initializer_list<value_type> l);
        template <class InputIt> paranoid_vector(InputIt first, InputIt last);
        paranoid_vector( size_type count, const T& value );

        void set_pool_preferred_max_size_bytes(size_t num_bytes);

        ~paranoid_vector();

        iterator begin() noexcept;
        const_iterator begin() const noexcept;
        const_iterator cbegin() const noexcept;

        iterator end() noexcept;
        const_iterator end() const noexcept;
        const_iterator cend() const noexcept;

        reverse_iterator rbegin() noexcept;
        const_reverse_iterator rbegin() const noexcept;
        const_reverse_iterator crbegin() const noexcept;

        reverse_iterator rend() noexcept;
        const_reverse_iterator rend() const noexcept;
        const_reverse_iterator crend() const noexcept;


        template <class InputIt> void assign(InputIt first, InputIt last);

        template< class... Args >
            void emplace_back( Args&&... args );

        void reserve (size_type n);
        size_type size() const;
        bool empty() const;

        void resize( size_type count );
        void resize (size_type n, const value_type& val);

        void push_back(const value_type& x);
        void push_back(value_type&& x);

        paranoid_vector& operator=( const paranoid_vector& other );
        reference at( size_type pos );
        const_reference at( size_type pos ) const;
        reference operator[]( size_type pos );
        const_reference operator[]( size_type pos ) const;
        T* data() noexcept;
        const T* data() const noexcept;
        void clear() noexcept;

        iterator erase( const_iterator pos );
        iterator erase( const_iterator first, const_iterator last);
        void pop_back();

        iterator insert( const_iterator pos, const_reference value );

        template< class InputIt >
            iterator insert( const_iterator pos, InputIt first, InputIt last );

        reference front();
        const_reference front() const;

        reference back();
        const_reference back() const;

    private:
        std::shared_ptr<paranoia_allocator<T>> allocator_;
        std::shared_ptr<ParanoiaPool> ppool_; // assumed to point at allocator_->ppool_ for lifespan of this vector.

        size_type num_elem_actual_ = 0;

        size_t buffer_size_bytes_ = 0; // Total buffer allocation size, in bytes.
        T* buffer_ = nullptr; // nullptr indicates we have no current allocation.

        void set_attached_buffer(T* new_buffer, size_type new_num_elem_capacity);

        void detach_current_buffer(
                int prot,
                T* & old_buffer,
                size_type & old_num_elem_actual
                );

        void deallocate_unattached_buffer(
                T* buffer,
                int prot);

        void create_replacement_buffer(
                const size_type new_buffer_elem_capacity,
                const T* old_buffer,
                const size_type old_buffer_num_elem,
                T* & new_buffer,
                T* & new_content_begin);

        T* create_uninit_buffer(const size_type num_elem_capacity);

        size_type remaining_elem_capacity() const;
};

template <typename T>
void paranoid_vector<T>::pop_back()
{
    assert(! empty());
    erase(end()-1);
}

template <typename T>
typename paranoid_vector<T>::iterator paranoid_vector<T>::erase( const_iterator first, const_iterator last )
{
    assert(!"TODO");
}

template <typename T>
typename paranoid_vector<T>::iterator paranoid_vector<T>::erase( const_iterator pos )
{
    assert(pos >= buffer_);
    assert(pos < buffer_ + num_elem_actual_);

    T* old_buffer;
    size_type old_num_elem;
    detach_current_buffer(PROT_READ, old_buffer, old_num_elem);

    const size_type new_num_elem = old_num_elem - 1;
    T* const new_buffer = create_uninit_buffer(new_num_elem);

    const size_type range1_num_elems = pos - old_buffer;
    const size_type range2_num_elems = old_num_elem - range1_num_elems - 1;

    T* dst = new_buffer;

    if (range1_num_elems > 0) {
        std::uninitialized_copy_n(old_buffer, range1_num_elems, dst);
        dst += range1_num_elems;
    }

    if (range2_num_elems > 0) {
        std::uninitialized_copy_n(old_buffer + range1_num_elems + 1, range2_num_elems, dst);
    }

    if (old_buffer) {
        deallocate_unattached_buffer(old_buffer, PROT_NONE);
    }

    set_attached_buffer(new_buffer, new_num_elem);

    return iterator(pos);
}

template <typename T>
void paranoid_vector<T>::set_attached_buffer(T* new_buffer, size_type new_num_elem_capacity)
{
    if (new_buffer) {
        assert(new_num_elem_capacity > 0);
    }
    else {
        assert(new_num_elem_capacity == 0);
    }

    buffer_ = new_buffer;
    num_elem_actual_ = new_num_elem_capacity;
    buffer_size_bytes_ = sizeof(T) * new_num_elem_capacity;
}

template <typename T>
T* paranoid_vector<T>::create_uninit_buffer(const size_type num_elem_capacity)
{
    if (num_elem_capacity == 0) {
        return nullptr;
    }
    else {
        ParanoiaPool & ppool = *(allocator_->ppool_);
        const size_t new_size_bytes = num_elem_capacity * sizeof(T);
        return reinterpret_cast<T*>(ppool.allocate(new_size_bytes));
    }
}

template <typename T>
void paranoid_vector<T>::deallocate_unattached_buffer(
        T* buffer,
        int prot)
{
    assert(buffer);

    ParanoiaPool & ppool = *(allocator_->ppool_);

    ppool.set_prot(buffer, prot);
    ppool.deallocate(buffer);
}

template <typename T>
void paranoid_vector<T>::create_replacement_buffer(
        const size_type new_elem_capacity,
        const T* old_buffer,
        const size_type old_buffer_num_elem,
        T* & new_buffer,
        T* & new_content_begin)
{
    new_buffer = create_uninit_buffer(new_elem_capacity);

    const size_t num_elem_to_copy = std::min(old_buffer_num_elem, new_elem_capacity);
    if (num_elem_to_copy > 0) {
        std::uninitialized_copy_n(old_buffer, num_elem_to_copy, new_buffer);
    }

    new_content_begin = new_buffer + num_elem_to_copy;
}

template <typename T>
void paranoid_vector<T>::detach_current_buffer(
        int prot,
        T* & old_buffer,
        size_type & old_num_elem_actual)
{
    old_buffer = buffer_;
    old_num_elem_actual = num_elem_actual_;

    set_attached_buffer(nullptr, 0);

    if (old_buffer) {
        ParanoiaPool & ppool = *(allocator_->ppool_);
        ppool.set_prot(old_buffer, prot);
    }
}

template <typename T>
void paranoid_vector<T>::resize (size_type count, const value_type& val)
{
    T* old_buffer;
    size_type old_num_elem;
    detach_current_buffer(PROT_READ, old_buffer, old_num_elem);

    T* new_buffer;
    T* new_content_begin;
    create_replacement_buffer(
            count,
            old_buffer, old_num_elem,
            new_buffer, new_content_begin);

    if (old_buffer) {
        deallocate_unattached_buffer(old_buffer, PROT_NONE);
    }

    const size_type num_additional_elem = std::min<size_type>(0, count - old_num_elem);
    for (size_type i = 0; i < num_additional_elem; ++i) {
        new (new_content_begin + i) T(val);
    }

    set_attached_buffer(new_buffer, count);
}

template <typename T>
void paranoid_vector<T>::resize( size_type count )
{
    T* old_buffer;
    size_type old_num_elem;
    detach_current_buffer(PROT_READ, old_buffer, old_num_elem);

    T* new_buffer;
    T* new_content_begin;
    create_replacement_buffer(
            count,
            old_buffer, old_num_elem,
            new_buffer, new_content_begin);

    if (old_buffer) {
        deallocate_unattached_buffer(old_buffer, PROT_NONE);
    }

    const size_type num_additional_elem = std::min<size_type>(0, count - old_num_elem);
    for (size_type i = 0; i < num_additional_elem; ++i) {
        new (new_content_begin + i) T();
    }

    set_attached_buffer(new_buffer, count);
}

template <typename T>
bool paranoid_vector<T>::empty() const
{
    return num_elem_actual_ == 0;
}

template <typename T>
template< class InputIt >
typename paranoid_vector<T>::iterator
    paranoid_vector<T>::insert(
        paranoid_vector<T>::const_iterator pos,
        InputIt first,
        InputIt last )
{
    const auto num_input_elem = std::distance(first, last);
    assert(num_input_elem >= 0);

    if (num_input_elem == 0) {
        return iterator(pos);
    }

    T* old_buffer;
    size_type old_num_elem;
    detach_current_buffer(PROT_READ, old_buffer, old_num_elem);

    assert(pos >= old_buffer);
    assert(pos < old_buffer + old_num_elem);

    const size_type new_num_elem = old_num_elem + num_input_elem;
    T* const new_buffer = create_uninit_buffer(new_num_elem);

    const size_type range1_num_elems = pos - old_buffer;
    const size_type range2_num_elems = old_num_elem - range1_num_elems;

    T* dst = new_buffer;

    if (range1_num_elems > 0) {
        std::uninitialized_copy_n(old_buffer, range1_num_elems, dst);
        dst += range1_num_elems;
    }

    const iterator insertion_point = dst;
    std::uninitialized_copy_n(first, num_input_elem, dst);
    dst += num_input_elem;

    if (range2_num_elems > 0) {
        std::uninitialized_copy_n(old_buffer + range1_num_elems, range2_num_elems, dst);
    }

    if (old_buffer) {
        deallocate_unattached_buffer(old_buffer, PROT_NONE);
    }

    set_attached_buffer(new_buffer, new_num_elem);

    return insertion_point;
}

template <typename T>
bool operator!=(const paranoid_vector<T> & lhs, const paranoid_vector<T> & rhs) {
    if (lhs.size() != rhs.size()) {
        return true;
    }

    const size_t num_elem = lhs.size();
    for (size_t i = 0; i < num_elem; ++i) {
        if (lhs[i] != rhs[i]) {
            return true;
        }
    }

    return false;
}

template <typename T>
bool operator==(const paranoid_vector<T> & lhs, const paranoid_vector<T> & rhs) {
    return ! (lhs != rhs);
}

template <typename T>
void paranoid_vector<T>::set_pool_preferred_max_size_bytes(size_t num_bytes)
{
    assert(allocator_);
    assert(allocator_->ppool_);
    allocator_->ppool_->set_preferred_max_bytes(num_bytes);
}

template <typename T>
paranoid_vector<T>::paranoid_vector(size_type count)
    : paranoid_vector(count, T())
{
}

template <typename T>
paranoid_vector<T>::paranoid_vector(size_type count, const T& value)
    : paranoid_vector()
{
    T* const new_buffer = create_uninit_buffer(count);

    for (size_type i = 0; i < count; ++i) {
        new (new_buffer + i) T(value);
    }

    set_attached_buffer(new_buffer, count);
}

template <typename T>
typename paranoid_vector<T>::reverse_iterator paranoid_vector<T>::rbegin() noexcept
{
    return reverse_iterator(end());
}

template <typename T>
typename paranoid_vector<T>::const_reverse_iterator paranoid_vector<T>::rbegin() const noexcept
{
    return const_reverse_iterator(end());
}

template <typename T>
typename paranoid_vector<T>::const_reverse_iterator paranoid_vector<T>::crbegin() const noexcept
{
    return const_reverse_iterator(end());
}

template <typename T>
typename paranoid_vector<T>::reverse_iterator paranoid_vector<T>::rend() noexcept
{
    return reverse_iterator(begin());
}

template <typename T>
typename paranoid_vector<T>::const_reverse_iterator paranoid_vector<T>::rend() const noexcept
{
    return  const_reverse_iterator(begin());
}

template <typename T>
typename paranoid_vector<T>::const_reverse_iterator paranoid_vector<T>::crend() const noexcept
{
    return const_reverse_iterator(begin());
}

template <typename T>
template <class InputIt>
paranoid_vector<T>::paranoid_vector(InputIt first, InputIt last)
    : paranoid_vector()
{
    assign(first, last);
}

template <typename T>
void paranoid_vector<T>::push_back(value_type&& x)
{
    T* old_buffer;
    size_type old_num_elem;
    detach_current_buffer(PROT_READ, old_buffer, old_num_elem);

    const size_type new_num_elem = old_num_elem + 1;

    T* new_buffer;
    T* new_content_begin;
    create_replacement_buffer(
            new_num_elem,
            old_buffer, old_num_elem,
            new_buffer, new_content_begin);

    if (old_buffer) {
        deallocate_unattached_buffer(old_buffer, PROT_NONE);
    }

    new (new_content_begin) T(x);

    set_attached_buffer(new_buffer, new_num_elem);
}

template <typename T>
template< class... Args >
void paranoid_vector<T>::emplace_back( Args&&... args )
{
    T* old_buffer;
    size_type old_num_elem;
    detach_current_buffer(PROT_READ, old_buffer, old_num_elem);

    const size_type new_num_elem = old_num_elem + 1;

    T* new_buffer;
    T* new_content_begin;
    create_replacement_buffer(
            new_num_elem,
            old_buffer, old_num_elem,
            new_buffer, new_content_begin);

    if (old_buffer) {
        deallocate_unattached_buffer(old_buffer, PROT_NONE);
    }

    new (new_content_begin) T(args...);

    set_attached_buffer(new_buffer, new_num_elem);
}

template <typename T>
typename paranoid_vector<T>::reference paranoid_vector<T>::front()
{
    assert(buffer_);
    return buffer_[0];
}

template <typename T>
typename paranoid_vector<T>::const_reference paranoid_vector<T>::front() const
{
    assert(buffer_);
    return buffer_[0];
}

template <typename T>
typename paranoid_vector<T>::reference paranoid_vector<T>::back()
{
    assert(buffer_);
    T* p_back = buffer_ + num_elem_actual_ - 1;
    return *p_back;
}

template <typename T>
typename paranoid_vector<T>::const_reference paranoid_vector<T>::back() const
{
    assert(buffer_);
    T* p_back = buffer_ + num_elem_actual_ - 1;
    return *p_back;
}

template <typename T>
void paranoid_vector<T>::reserve (paranoid_vector<T>::size_type n)
{
    // TODO:
    // Right now we're not catering to this usage profile, and we
    // have too much code written that assumes capacity() == size()
    // for it to be a really quick feature to add.
}

template <typename T>
paranoid_vector<T>::paranoid_vector(std::initializer_list<value_type> l)
    : paranoid_vector()
{
    this->assign(l.begin(), l.end());
}

template <typename T>
typename paranoid_vector<T>::iterator paranoid_vector<T>::begin() noexcept
{
    return buffer_;
}

template <typename T>
typename paranoid_vector<T>::const_iterator paranoid_vector<T>::begin() const noexcept
{
    return buffer_;
}

template <typename T>
typename paranoid_vector<T>::const_iterator paranoid_vector<T>::cbegin() const noexcept
{
    return buffer_;
}

template <typename T>
typename paranoid_vector<T>::iterator paranoid_vector<T>::end() noexcept
{
    // Technically this could cause pointer overflow (i.e., wraparound), but
    // in practice that won't happen.
    return buffer_ + num_elem_actual_;
}

template <typename T>
typename paranoid_vector<T>::const_iterator paranoid_vector<T>::end() const noexcept
{
    // Technically this could cause pointer overflow (i.e., wraparound), but
    // in practice that won't happen.
    return buffer_ + num_elem_actual_;
}

template <typename T>
typename paranoid_vector<T>::const_iterator paranoid_vector<T>::cend() const noexcept
{
    // Technically this could cause pointer overflow (i.e., wraparound), but
    // in practice that won't happen.
    return buffer_ + num_elem_actual_;
}

template <typename T>
paranoid_vector<T>::paranoid_vector(const paranoid_vector<T>& other)
    : paranoid_vector(other.allocator_)
{
    (*this) = other;
}

template <typename T>
typename paranoid_vector<T>::size_type paranoid_vector<T>::size() const {
    return num_elem_actual_;
}

template <typename T>
paranoid_vector<T>::~paranoid_vector()
{
    clear();
}

template <typename T>
typename paranoid_vector<T>::size_type  paranoid_vector<T>::remaining_elem_capacity() const {
    const size_t bytes_used = num_elem_actual_ * sizeof(T);
    const size_t bytes_remaining = buffer_size_bytes_ - bytes_used;

    if (bytes_remaining == 0) {
        return 0;
    }
    else {
        return sizeof(T) / bytes_remaining;
    }
}

template <typename T>
void paranoid_vector<T>::push_back(const paranoid_vector<T>::value_type& x) {
    T* old_buffer;
    size_type old_num_elem;
    detach_current_buffer(PROT_READ, old_buffer, old_num_elem);

    const size_type new_num_elem = old_num_elem + 1;

    T* new_buffer;
    T* new_content_begin;
    create_replacement_buffer(
            new_num_elem,
            old_buffer, old_num_elem,
            new_buffer, new_content_begin);

    if (old_buffer) {
        deallocate_unattached_buffer(old_buffer, PROT_NONE);
    }

    new (new_content_begin) T(x);

    set_attached_buffer(new_buffer, new_num_elem);
}

template <typename T>
typename paranoid_vector<T>::reference paranoid_vector<T>::at( paranoid_vector<T>::size_type pos ) {
    if (pos > num_elem_actual_)
    {
        std::ostringstream os;
        os << "size()=" << size() << " but pos=" << pos;
        throw std::out_of_range(os.str());
    }

    return *(buffer_ + pos);
}

template <typename T>
typename paranoid_vector<T>::const_reference paranoid_vector<T>::at( paranoid_vector<T>::size_type pos ) const {
    if (pos > num_elem_actual_)
    {
        std::ostringstream os;
        os << "size()=" << size() << " but pos=" << pos;
        throw std::out_of_range(os.str());
    }

    return *(buffer_ + pos);
}

template <typename T>
typename paranoid_vector<T>::reference paranoid_vector<T>::operator[]( paranoid_vector<T>::size_type pos ) {
    return at(pos);
}

template <typename T>
typename paranoid_vector<T>::const_reference paranoid_vector<T>::operator[]( paranoid_vector<T>::size_type pos ) const {
    return at(pos);
}

template <typename T>
T* paranoid_vector<T>::data() noexcept {
    return buffer_;
}

template <typename T>
const T* paranoid_vector<T>::data() const noexcept {
    return buffer_;
}

template <typename T>
void paranoid_vector<T>::clear() noexcept {
    T* old_buffer;
    size_type old_num_elem;
    detach_current_buffer(PROT_READ, old_buffer, old_num_elem);

    if (old_buffer) {
        deallocate_unattached_buffer(old_buffer, PROT_NONE);
    }
}

template <typename T>
paranoid_vector<T>::paranoid_vector(std::shared_ptr<allocator_type> allocator)
{
    assert(allocator);
    assert(allocator->ppool_);

    allocator_ = allocator;
    ppool_ = allocator_->ppool_;
}

template <typename T>
paranoid_vector<T>& paranoid_vector<T>::operator=( const paranoid_vector& other ) {
    T* old_buffer;
    size_type old_num_elem;
    detach_current_buffer(PROT_NONE, old_buffer, old_num_elem);

    const size_type new_num_elem = other.size();
    T* new_buffer = create_uninit_buffer(new_num_elem);

    if (new_num_elem > 0) {
        assert(other.buffer_);
        assert(new_buffer);
        std::uninitialized_copy_n(other.buffer_, new_num_elem, new_buffer);
    }

    if (old_buffer) {
        deallocate_unattached_buffer(old_buffer, PROT_NONE);
    }

    set_attached_buffer(new_buffer, new_num_elem);

    return *this;
}

template <typename T>
template <typename InputIt>
void paranoid_vector<T>::assign(InputIt first, InputIt last)
{
    // FIXME: This should do a SFINAE check to confirm that InputIt is truly an input iterator

    const auto new_num_elem = std::distance(first, last);

    T* old_buffer;
    size_type old_num_elem;
    detach_current_buffer(PROT_NONE, old_buffer, old_num_elem);

    if (old_buffer) {
        deallocate_unattached_buffer(old_buffer, PROT_NONE);
    }

    T* new_buffer;

    if (new_num_elem > 0) {
        new_buffer = create_uninit_buffer(new_num_elem);
        std::uninitialized_copy_n(first, new_num_elem, new_buffer);
    }
    else {
        new_buffer = nullptr;
    }

    set_attached_buffer(new_buffer, new_num_elem);
}

template <typename T>
typename paranoid_vector<T>::iterator paranoid_vector<T>::insert(
        paranoid_vector<T>::const_iterator pos,
        paranoid_vector<T>::const_reference val)
{
    T* old_buffer;
    size_type old_num_elem;
    detach_current_buffer(PROT_READ, old_buffer, old_num_elem);

    assert(pos >= old_buffer);
    assert(pos < old_buffer + old_num_elem);

    const size_type new_num_elem = old_num_elem + 1;
    T* const new_buffer = create_uninit_buffer(new_num_elem);

    const size_type range1_num_elems = pos - old_buffer;
    const size_type range2_num_elems = old_num_elem - range1_num_elems;

    T* dst = new_buffer;

    if (range1_num_elems > 0) {
        std::uninitialized_copy_n(old_buffer, range1_num_elems, dst);
        dst += range1_num_elems;
    }

    const iterator insertion_point = dst;
    new (dst) T(val);
    ++dst;

    if (range2_num_elems > 0) {
        std::uninitialized_copy_n(old_buffer + range1_num_elems, range2_num_elems, dst);
    }

    if (old_buffer) {
        deallocate_unattached_buffer(old_buffer, PROT_NONE);
    }

    set_attached_buffer(new_buffer, new_num_elem);

    return insertion_point;
}

#define DECLARE_PARANOID_VECTOR_SPECIALIZATION(ELEM_TYPE) \
namespace std { \
    extern template class vector< ELEM_TYPE , allocator< ELEM_TYPE  > >; \
}

#define DEFINE_PARANOID_VECTOR_SPECIALIZATION(ELEM_TYPE) \
namespace std { \
    template<> \
        class vector< ELEM_TYPE , allocator< ELEM_TYPE  > > \
        : public paranoid_vector< ELEM_TYPE > { \
            public: \
                using base_class = paranoid_vector< ELEM_TYPE >; \
                using value_type = base_class::value_type; \
 \
                using allocator_type         = base_class::allocator_type; \
                using size_type              = base_class::size_type; \
                using difference_type        = base_class::difference_type; \
                using reference              = base_class::reference; \
                using const_reference        = base_class::const_reference; \
                using pointer                = base_class::pointer; \
                using const_pointer          = base_class::const_pointer; \
                using iterator               = base_class::iterator; \
                using const_iterator         = base_class::const_iterator; \
                using reverse_iterator       = base_class::reverse_iterator; \
                using const_reverse_iterator = base_class::const_reverse_iterator; \
 \
                vector(size_t pool_preferred_max_size_bytes)                                           : base_class(pool_preferred_max_size_bytes) {} \
                vector(std::shared_ptr<allocator_type> allocator = std::make_shared<allocator_type>()) : base_class(allocator) {} \
                vector(const vector<value_type>& other)                                            : base_class(other) {} \
                vector(std::initializer_list<value_type> l)                                            : base_class(l) {} \
                vector& operator=( const vector& other ) { base_class::operator=(other); return *this; } \
 \
                int foozle; \
        }; \
}

