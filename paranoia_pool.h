#pragma once

#include <sys/mman.h>
#include <map>
#include <queue>

class ParanoiaPool {
    public:
        ParanoiaPool(size_t preferred_max_bytes);
        virtual ~ParanoiaPool();

        void* allocate(size_t num_bytes, int initial_prot = PROT_READ | PROT_WRITE);
        void deallocate(void* p);
        void set_prot(void* p, int prot);
        int get_prot(void* p);

    private:
        const size_t preferred_max_bytes_;
        static const size_t s_page_size_;

        struct AllocDetails {
            AllocDetails() = default;

            AllocDetails(
                    void* addr,
                    size_t num_bytes,
                    int prot);

            void* addr;
            size_t num_bytes;
            int prot;
        };

        std::map<void*,AllocDetails> live_allocs_;
        std::queue<AllocDetails> stale_allocs_;
        size_t total_alloc_bytes_ = 0;

        static size_t get_page_size();
        static size_t num_pages_needed(size_t num_bytes);
        void gc_as_needed(size_t upcoming_alloc_bytes);
        void gc_one_alloc();
};
