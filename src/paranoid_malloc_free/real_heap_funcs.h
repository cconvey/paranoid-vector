#pragma once

#include <cstdlib>

extern void* (*real_malloc)(size_t);
extern void (*real_free)(void*);

void init_real_heap_funcs();
