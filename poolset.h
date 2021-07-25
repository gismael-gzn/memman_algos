#ifndef POOLSET_ALLOCATOR_H__
#define POOLSET_ALLOCATOR_H__

#include "pool.h"

typedef struct poolset_t poolset_t;

size_t poolset_sizeof(size_t poolno);
size_t poolset_smallestsize(poolset_t* chain);
size_t poolset_biggestsize(poolset_t* chain);

poolset_t *poolset_init(void *mem, size_t memsize, size_t poolmem, size_t step);

#endif //POOLSET_ALLOCATOR_H__
