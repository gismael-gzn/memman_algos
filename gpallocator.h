#ifndef GENERAL_PURPOSE_ALLOCATOR_H__
#define GENERAL_PURPOSE_ALLOCATOR_H__

#include "poolset.h"
#include <string.h>

struct memman_hooks
{
	malloc_impl* mallochook;
	realloc_impl* reallochook;
	free_impl* freehook;
};

typedef struct gpallocator_t gpallocator_t;

gpallocator_t* gpallocator_new(struct memman_hooks hooks, size_t sets, 
size_t step, size_t max_block, void* id);

void gpallocator_del(gpallocator_t* allocator);

void* gpallocator_malloc(gpallocator_t* allocator, size_t n);

void* gpallocator_realloc(void* payload);

void* gpallocator_free(void* payload);

size_t gpallocated_size(void* payload);

#endif //GENERAL_PURPOSE_ALLOCATOR_H__
