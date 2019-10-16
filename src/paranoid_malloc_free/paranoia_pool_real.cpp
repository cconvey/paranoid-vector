#include "paranoia_pool_real.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <unistd.h>

using namespace std;

ParanoiaPool_real::AllocDetails::AllocDetails(
        void* addr,
        size_t num_bytes,
        int prot)
    : addr(addr), num_bytes(num_bytes), prot(prot)
{
}

void ParanoiaPool_real::set_preferred_max_bytes(size_t num_bytes)
{
    preferred_max_bytes_ = num_bytes;
    gc_as_needed(0);
}

void ParanoiaPool_real::gc_as_needed(size_t upcoming_alloc_bytes)
{
    while ((total_alloc_bytes_ + upcoming_alloc_bytes > preferred_max_bytes_) &&
            (! stale_allocs_.empty()))
    {
        gc_one_alloc();
    }
}

//const size_t ParanoiaPool_real::s_page_size_ = ParanoiaPool_real::get_page_size();
const size_t ParanoiaPool_real::s_page_size_ = 4096;

size_t ParanoiaPool_real::get_page_size()
{
    long val = sysconf(_SC_PAGESIZE);
    if (val == -1) {
        assert(! "Unable to determine page size.");
    }

    return size_t(val);
}

ParanoiaPool_real::ParanoiaPool_real(
        size_t preferred_max_bytes) :
    preferred_max_bytes_(preferred_max_bytes)
{
}

ParanoiaPool_real::~ParanoiaPool_real() {
    if (! live_allocs_.empty()) {
        assert(! "ParanoiaPool_real has outstanding allocations when destroyed.");
    }

    while (! stale_allocs_.empty()) {
        gc_one_alloc();
    }
}

void ParanoiaPool_real::gc_one_alloc() {
    assert(! stale_allocs_.empty());

    AllocDetails & victim = stale_allocs_.front();

    // We should probably restore normal access to the victim pages before
    // calling free(...).
    if (victim.prot != PROT_READ|PROT_WRITE) {
        if (mprotect(victim.addr, victim.num_bytes, PROT_READ|PROT_WRITE)) {
            assert(!"Failed call to mprotect.");
        }
    }

    real_free(victim.addr);

    assert(total_alloc_bytes_ >= victim.num_bytes);
    total_alloc_bytes_ -= victim.num_bytes;

    stale_allocs_.pop();
}

void* ParanoiaPool_real::allocate(size_t num_bytes, int initial_prot) {
    assert(num_bytes > 0);

    const size_t new_alloc_num_pages = num_pages_needed(num_bytes);
    const size_t new_alloc_total_bytes = new_alloc_num_pages * s_page_size_;

    gc_as_needed(new_alloc_total_bytes);

    void* p = aligned_alloc(s_page_size_, new_alloc_total_bytes);
    if (!p) {
        assert(!"aligned_alloc failed.");
        return nullptr;
    }

    const auto iter = live_allocs_.find(p);
    assert(iter == live_allocs_.end());

    live_allocs_[p] = AllocDetails(p, new_alloc_total_bytes, initial_prot);

    if (initial_prot != (PROT_READ | PROT_WRITE)) {
        set_prot(p, initial_prot);
    }

    total_alloc_bytes_ += new_alloc_total_bytes;
    return p;
}

void ParanoiaPool_real::deallocate(void* p) {
    if (!p) {
        return;
    }

    const auto iter = live_allocs_.find(p);
    if (iter == live_allocs_.end()) {
        // If may be a pointer from before our malloc function was interposed.
        //real_free(p);
        //return;
        assert(! "pointer is not managed by this ParanoiaPool_real");
        abort();
    }

    set_prot(p, PROT_NONE);

    stale_allocs_.push(iter->second);
    live_allocs_.erase(iter);

    // Just in case we were already over preferred capacity.
    gc_as_needed(0);
}

int ParanoiaPool_real::get_prot(void* p) {
    const auto iter = live_allocs_.find(p);
    if (iter == live_allocs_.end()) {
        assert(! "pointer not managed by this ParanoiaPool_real.");
        abort();
    }

    return iter->second.prot;
}

void ParanoiaPool_real::set_prot(void* p, int prot) {
    const auto iter = live_allocs_.find(p);
    if (iter == live_allocs_.end()) {
        assert(! "pointer not managed by this ParanoiaPool_real.");
        abort();
    }

    if (iter->second.prot == prot) {
        return;
    }

    const size_t num_bytes = iter->second.num_bytes;

    if (mprotect(p, num_bytes, prot)) {
        assert(!"Failed to call mprotect.");
        abort();
    }

    iter->second.prot = prot;
}

size_t ParanoiaPool_real::num_pages_needed(size_t num_bytes) {
    size_t full_pages = num_bytes / s_page_size_;
    if ((num_bytes % s_page_size_) > 0) {
        return full_pages + 1;
    }
    else {
        return full_pages;
    }
}

