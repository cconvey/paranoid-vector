#include "paranoia_pool.h"

#include "util.h"

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

static const size_t BILLION = 1000 * 1000  * 1000;

static size_t get_ideal_max_allocs()
{
    const long max_map_count = get_vm_max_map_count();
    const long ideal_max = max_map_count / 2;
    return checked_cast<size_t>(ideal_max);
}

static const size_t GLOBAL_DEFAULT_POOL_IDEAL_MAX_BYTES = 25*BILLION;
static const size_t GLOBAL_DEFAULT_POOL_IDEAL_MAX_ALLOCS = get_ideal_max_allocs();
static const size_t PAGE_SIZE = get_page_size();

const std::shared_ptr<ParanoiaPool> g_paranoia_default_pool = make_shared<ParanoiaPool>(
        GLOBAL_DEFAULT_POOL_IDEAL_MAX_BYTES,
        GLOBAL_DEFAULT_POOL_IDEAL_MAX_ALLOCS);


ParanoiaPool::AllocDetails::AllocDetails(
        void* addr,
        size_t num_bytes,
        int prot)
    : addr(addr), num_bytes(num_bytes), prot(prot)
{
}

void ParanoiaPool::set_preferred_max_bytes(size_t num_bytes)
{
    preferred_max_bytes_ = num_bytes;
    gc_as_needed(0);
}

void ParanoiaPool::gc_as_needed(size_t upcoming_alloc_bytes)
{
    while ((total_alloc_bytes_ + upcoming_alloc_bytes > preferred_max_bytes_) &&
            (! stale_allocs_.empty()))
    {
        gc_one_alloc();
    }

    const size_t num_upcoming_allocs = live_allocs_.size() + stale_allocs_.size() + 1;

    if (num_upcoming_allocs > preferred_max_allocs_) {
        const size_t num_excess_allocs = num_upcoming_allocs - preferred_max_allocs_;
        const size_t num_allocs_to_gc = std::min<size_t>(num_excess_allocs, stale_allocs_.size());
        for (size_t i = 0; i < num_allocs_to_gc; ++i) {
            gc_one_alloc();
        }

    }
}

ParanoiaPool::ParanoiaPool(size_t preferred_max_bytes, size_t preferred_max_allocs) :
    preferred_max_bytes_(preferred_max_bytes),
    preferred_max_allocs_(preferred_max_allocs)
{
#if PARANOIA_LOGGING
    cout << __PRETTY_FUNCTION__ << " :"
        << " this=" << HexPtr(this)
        << endl;
#endif
}

ParanoiaPool::~ParanoiaPool() {
#if PARANOIA_LOGGING
    cout << __PRETTY_FUNCTION__ << " :"
        << " this=" << HexPtr(this)
        << endl;
#endif

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

#if PARANOIA_LOGGING
    cout << __PRETTY_FUNCTION__ << " :"
        << " this=" << HexPtr(this)
        << " victim.addr=" << HexPtr(victim.addr)
        << endl;
#endif

    // We should probably restore normal access to the victim pages before
    // calling free(...).
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

#if PARANOIA_LOGGING
    cout << __PRETTY_FUNCTION__ << " :"
        << " ENTER"
        << " this=" << HexPtr(this)
        << " num_byts=" << num_bytes
        << endl;
#endif

    const size_t new_alloc_num_pages = num_pages_needed(num_bytes);
    const size_t new_alloc_total_bytes = new_alloc_num_pages * PAGE_SIZE;

    gc_as_needed(new_alloc_total_bytes);

    void* p = aligned_alloc(PAGE_SIZE, new_alloc_total_bytes);
    if (!p) {
        const string e = std::strerror(errno);
        ostringstream os;
        os << ": "
            << " PAGE_SIZE=" << PAGE_SIZE
            << " new_alloc_total_bytes=" << new_alloc_total_bytes;
        throw std::runtime_error(os.str());
    }

    const auto iter = live_allocs_.find(p);
    assert(iter == live_allocs_.end());

    live_allocs_[p] = AllocDetails(p, new_alloc_total_bytes, initial_prot);

    if (initial_prot != (PROT_READ | PROT_WRITE)) {
        set_prot(p, initial_prot);
    }

    total_alloc_bytes_ += new_alloc_total_bytes;

#if PARANOIA_LOGGING
    cout << __PRETTY_FUNCTION__ << " :"
        << " RETURN=" << HexPtr(p)
        << endl;
#endif

    return p;
}

void ParanoiaPool::deallocate(void* p) {
#if PARANOIA_LOGGING
    cout << __PRETTY_FUNCTION__ << " :"
        << " this=" << HexPtr(this)
        << " p=" << HexPtr(p)
        << endl;
#endif

    const auto iter = live_allocs_.find(p);
    if (iter == live_allocs_.end()) {
        assert(! "pointer is not managed by this ParanoiaPool");
        abort();
    }

    set_prot(p, PROT_NONE);

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
#if PARANOIA_LOGGING
    cout << __PRETTY_FUNCTION__ << " :"
        << " this=" << HexPtr(this)
        << " p=" << HexPtr(p)
        << endl;
#endif

    const auto iter = live_allocs_.find(p);
    if (iter == live_allocs_.end()) {
        assert(! "pointer not managed by this ParanoiaPool.");
        abort();
    }

    if (iter->second.prot == prot) {
        return;
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
    size_t full_pages = num_bytes / PAGE_SIZE;
    if ((num_bytes % PAGE_SIZE) > 0) {
        return full_pages + 1;
    }
    else {
        return full_pages;
    }
}
