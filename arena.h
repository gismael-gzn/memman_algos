#ifndef ARENA_ALLOCATOR_H__
#define ARENA_ALLOCATOR_H__

#include <stdlib.h>
#include "extrastddef.h"

typedef void* malloc_impl(size_t);
typedef void* realloc_impl(void*, size_t);
typedef void free_impl(void*);

typedef struct arena_t arena_t;

size_t arena_sizeof(void);

size_t bytes_to_align(void* ptr, size_t alignment);

void* align_pointer(void* ptr, size_t alignment);

size_t arena_capacity(arena_t* reg);

size_t arena_size(arena_t* reg);

arena_t* arena_init(void* mem, size_t mem_size, size_t alignment);

arena_t* arena_new(malloc_impl* mallochook, size_t mem_size, size_t alignment);

void* arena_malloc(arena_t* reg, size_t n);

void* arena_malloc_quick(arena_t* reg, size_t n);

#endif //ARENA_ALLOCATOR_H__
