#include "paranoia_pool.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

using namespace std;

ParanoiaPool::AllocDetails::AllocDetails(
        void* addr,
        size_t num_bytes,
        int prot)
    : addr(addr), num_bytes(num_bytes), prot(prot)
{
}

void ParanoiaPool::gc_as_needed(size_t upcoming_alloc_bytes)
{
    while ((total_alloc_bytes_ + upcoming_alloc_bytes > preferred_max_bytes_) &&
            (! stale_allocs_.empty()))
    {
        gc_one_alloc();
    }
}

const size_t ParanoiaPool::s_page_size_ = ParanoiaPool::get_page_size();

size_t ParanoiaPool::get_page_size()
{
    long val = sysconf(_SC_PAGESIZE);
    if (val == -1) {
        const string e = std::strerror(errno);
        ostringstream os;
        os << "Unable to determine page size: " << e;
        throw std::runtime_error(os.str());
    }

    return size_t(val);
}

ParanoiaPool::ParanoiaPool(size_t preferred_max_bytes)
    : preferred_max_bytes_(preferred_max_bytes)
{
}

ParanoiaPool::~ParanoiaPool() {
    if (! live_allocs_.empty()) {
        cerr << __PRETTY_FUNCTION__ << " :"
            << " outstanding allocations: " << live_allocs_.size()
            << endl;
        assert(! "ParanoiaPool has outstanding allocations when destroyed.");
    }

    while (! stale_allocs_.empty()) {
        gc_one_alloc();
    }
}

void ParanoiaPool::gc_one_alloc() {
    assert(! stale_allocs_.empty());

    AllocDetails & victim = stale_allocs_.front();

    cout << __PRETTY_FUNCTION__ << " :"
        << " victim.addr=" << hex << showbase << victim.addr << dec
        << endl;

    if (victim.prot != PROT_READ|PROT_WRITE) {
        if (mprotect(victim.addr, victim.num_bytes, PROT_READ|PROT_WRITE)) {
            const string e = std::strerror(errno);
            ostringstream os;
            os << "Failed call to mprotect: " << e;
            throw std::runtime_error(os.str());
        }
    }

    free(victim.addr);

    assert(total_alloc_bytes_ >= victim.num_bytes);
    total_alloc_bytes_ -= victim.num_bytes;

    stale_allocs_.pop();
}

void* ParanoiaPool::allocate(size_t num_bytes, int initial_prot) {
    assert(num_bytes > 0);

    const size_t new_alloc_num_pages = num_pages_needed(num_bytes);
    const size_t new_alloc_total_bytes = new_alloc_num_pages * s_page_size_;

    gc_as_needed(new_alloc_total_bytes);

    void* p = nullptr;
    int rc = posix_memalign(&p, s_page_size_, num_bytes);
    switch (rc) {
        case 0:
            break;
        case EINVAL:
            cerr << __PRETTY_FUNCTION__ << " :"
                << " EINVAL for num_bytes=" << num_bytes
                << endl;
            assert(! "Failed memory allocation: EINVAL");
            return nullptr;
        case ENOMEM:
            cerr << __PRETTY_FUNCTION__ << " :"
                << " ENOMEM for num_bytes=" << num_bytes
                << endl;
            assert(! "Failed memory allocation: EINVAL");
            return nullptr;
        default:
            assert(! "Unexpected return code.");
            return nullptr;
    }

    if (p) {
        const auto iter = live_allocs_.find(p);
        assert(iter == live_allocs_.end());

        live_allocs_[p] = AllocDetails(p, new_alloc_total_bytes, initial_prot);

        if (initial_prot != (PROT_READ | PROT_WRITE)) {
            set_prot(p, initial_prot);
        }
    }

    total_alloc_bytes_ += new_alloc_total_bytes;

    cout << __PRETTY_FUNCTION__ << " :"
        << " return=" << hex << showbase << p << dec
        << endl;

    return p;
}

void ParanoiaPool::deallocate(void* p) {
    cout << __PRETTY_FUNCTION__ << " :"
        << " p=" << hex << showbase << p << dec
        << endl;

    const auto iter = live_allocs_.find(p);
    if (iter == live_allocs_.end()) {
        assert(! "pointer is not managed by this ParanoiaPool");
        abort();
    }

    stale_allocs_.push(iter->second);
    live_allocs_.erase(iter);

    // Just in case we were already over preferred capacity.
    gc_as_needed(0);
}

int ParanoiaPool::get_prot(void* p) {
    const auto iter = live_allocs_.find(p);
    if (iter == live_allocs_.end()) {
        assert(! "pointer not managed by this ParanoiaPool.");
        abort();
    }

    return iter->second.prot;
}

void ParanoiaPool::set_prot(void* p, int prot) {
    const auto iter = live_allocs_.find(p);
    if (iter == live_allocs_.end()) {
        assert(! "pointer not managed by this ParanoiaPool.");
        abort();
    }

    const size_t num_bytes = iter->second.num_bytes;

    if (mprotect(p, num_bytes, prot)) {
        const string e = std::strerror(errno);
        ostringstream os;
        os << "Failed call to mprotect: " << e;
        throw std::runtime_error(os.str());
    }

    iter->second.prot = prot;
}

size_t ParanoiaPool::num_pages_needed(size_t num_bytes) {
    size_t full_pages = num_bytes / s_page_size_;
    if ((num_bytes % s_page_size_) > 0) {
        return full_pages + 1;
    }
    else {
        return full_pages;
    }
}
