#include <cstdint>
#include <thread>
#include <mutex>
#include <cassert>

#include "real_heap_funcs.h"
#include "paranoia_pool_real.h"

extern "C" {
    void* malloc(size_t size);
    void free(void* p);
}

//static void lib_init() __attribute__((constructor));
//static void lib_deinit() __attribute__((destructor));

static const size_t BILLION = 1000 * 1000 * 1000;
static const size_t SIZE_OF_GLOBAL_DEFAULT_POOL = 20 * BILLION;
static ParanoiaPool_real * g_pool;
static std::mutex * g_mutex;

static bool init_complete = false;

static void ensure_lib_init() {
    if (! init_complete) {
        init_real_heap_funcs();

        void* p;
        p = real_malloc(sizeof(ParanoiaPool_real));
        assert(p);
        g_pool = new (p) ParanoiaPool_real(SIZE_OF_GLOBAL_DEFAULT_POOL);

        p = real_malloc(sizeof(std::mutex));
        assert(p);
        g_mutex = new (p) std::mutex();

        init_complete = true;
    }
}

static void lib_deinit() {
    if (g_pool) {
        real_free(g_pool);
        g_pool = nullptr;
    }

    if (g_mutex) {
        real_free(g_mutex);
        g_mutex = nullptr;
    }
}

void* malloc(size_t size)
{
    ensure_lib_init();
    std::lock_guard<std::mutex> lock(*g_mutex);
    return g_pool->allocate(size);
}

void free(void* p) {
    if (!p) {
        return;
    }

    ensure_lib_init();
    std::lock_guard<std::mutex> lock(*g_mutex);
    g_pool->deallocate(p);
}
