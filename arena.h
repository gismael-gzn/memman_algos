#ifndef ARENA_ALLOCATOR_H__
#define ARENA_ALLOCATOR_H__

/* Region based / arena allocator. 
Allocate a big chunk of memory to make sub-allocations within said block.
arenas are freeable by using the inverse of the malloc-like function from which 
the pool variable got its memory (check the initializer functions).
For example if malloc is used, then to free the pool simple use free(arena) */

#include <stdlib.h>
#include "extrastddef.h"

/* typedef for memman function pointers */
typedef void* malloc_impl(size_t);
typedef void* realloc_impl(void*, size_t);
typedef void free_impl(void*);

/* arenas are handled as opaque datatypes by code-user */
typedef struct arena_t arena_t;

/* returns the size of the arena_t complete type */
size_t arena_sizeof(void);

/* returns the number of required bytes to align ptr as 'alignment' */
size_t bytes_to_align(void* ptr, size_t alignment);

/* if ptr is already align, it returns ptr,
else, returns a ptr to the nearest next aligned address.
No bound checking is performed */
void* align_pointer(void* ptr, size_t alignment);

/* returns the number of bytes available in the arena */
size_t arena_capacity(arena_t* reg);

/* returns the total capacity of the arena in bytes */
size_t arena_size(arena_t* reg);

/* initialize an arena with mem pointing to an allocation of size mem_size
the 'alignment' argument is so that the first allocation has that alignment,
if mem_size is less than sizeof(arena_t) + bytes_to_align_first_allocation
the function returns NULL;
 */
arena_t* arena_init(void* mem, size_t mem_size, size_t alignment);

/* it takes a pointer to a malloc-like funcion to allocated mem_size bytes 
and initialize the arena. 
If mallochook is NULL, then stdlib's malloc is used */
arena_t* arena_new(malloc_impl* mallochook, size_t mem_size, size_t alignment);

/* if the region has n bytes of capacity, then it returns an address which is in
the range of [mem+sizeof(arena_t)+bytes_to_align_first_allocation, mem+mem_size]
(check arena_init function). Otherwise, returns NULL */
void* arena_malloc(arena_t* reg, size_t n);

/* returns the address of the last_alloc+n without checking the region capacity. */
void* arena_malloc_quick(arena_t* reg, size_t n);

#endif //ARENA_ALLOCATOR_H__
