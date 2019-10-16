#include "real_heap_funcs.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <cassert>

void* (*real_malloc)(size_t) = nullptr;
void (*real_free)(void*) = nullptr;

void init_real_heap_funcs()
{
    real_malloc = reinterpret_cast<void* (*)(size_t)>(dlsym(RTLD_NEXT, "malloc"));
    assert(real_malloc);

    real_free = reinterpret_cast<void (*)(void*)>(dlsym(RTLD_NEXT, "free"));
    assert(real_free);
}
