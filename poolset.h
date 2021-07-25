#ifndef POOLSET_ALLOCATOR_H__
#define POOLSET_ALLOCATOR_H__

#include "pool.h"

typedef struct poolchain_t poolchain_t;

size_t poolchain_memsize(size_t poolno);
size_t poolchain_smallestsize(poolchain_t* chain);
size_t poolchain_biggestsize(poolchain_t* chain);

poolchain_t *poolchain_init(void *mem, size_t memsize, size_t pool_bytes,
							size_t step);

#endif //POOLSET_ALLOCATOR_H__
