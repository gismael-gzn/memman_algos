#ifndef ARENA_ALLOCATOR_H__
#define ARENA_ALLOCATOR_H__

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

#define isnull(ptr) (ptr == NULL)

#define byteptr(ptr) ((uint8_t*)ptr)

#define ptroff(begin, end) (byteptr(end)-byteptr(begin))

#define to_nearest_multiple(x, mul) ((x==0?1:x)%(mul)==0? 0: ((mul)-((x)%(mul))))

typedef void* malloc_impl(size_t);
typedef void* realloc_impl(void*, size_t);
typedef void free_impl(void*);

typedef uint8_t byte_t;

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
