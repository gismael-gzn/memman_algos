#ifndef POOLSET_ALLOCATOR_H__
#define POOLSET_ALLOCATOR_H__

#include "pool.h"

typedef struct poolset_t poolset_t;

size_t poolset_sizeof(size_t poolno);
size_t poolset_smallestsize(poolset_t* chain);
size_t poolset_biggestsize(poolset_t* chain);

poolset_t* poolset_new(malloc_impl* mallochook, size_t step, size_t max_block, void* id);

void* poolset_pull(poolset_t* set, size_t n);

void* poolset_pull_quick(poolset_t* set, size_t n);

void* poolset_push(poolset_t* set, void* payload);

#endif //POOLSET_ALLOCATOR_H__
