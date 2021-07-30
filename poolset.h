#ifndef POOLSET_ALLOCATOR_H__
#define POOLSET_ALLOCATOR_H__

#include "pool.h"

/* Size-based pool allocator
Allocate a large enough memory region to then sub-allocate a finite quantity of 
various types of fixed-size blocks.
poolsets are freeable by using the inverse of the malloc-like function from which 
the poolset variable got its memory (check the initializer functions).
For example if malloc is used, then to free the pool simple use free(poolset) */

/* poolset opaque type */
typedef struct poolset_t poolset_t;

/* returns the size of the poolset_t complete type */
size_t poolset_sizeof(size_t poolno);

/* returns the smallest size which the poolset can allocate */
size_t poolset_smallestsize(poolset_t* set);

/* returns the biggest size which the poolset can allocate */
size_t poolset_biggestsize(poolset_t* set);

/* a poolset is a collection of pools that differ differ in their segment_size 
by a linear factor 'step', if step is not a multiple of a granule's size, then
it's re-adjusted to be so. The first pool in the collection has size step,
the second has 2*step, and so on up to max_block (which is also a multiple of
the size of a granule). each pool gets initialized with a block 8 times max_block.
If mallochook is NULL, then stdlib's malloc is used.
A possible fail point of this function is if for some reason mallochook doesn't
return enough space to allocate all the pools.
check cell.h for more info about the last argument.
 */
poolset_t* poolset_new(malloc_impl* mallochook, size_t step, size_t max_block, void* owner);

/* maps n to a pool index and allocates a block from it, returns NULL if the pool
has no more free-blocks or if n maps to a non-existent pool index. 
qurying for a block of size 0 is as querying for a block of size 1 */
void* poolset_pull(poolset_t* set, size_t n);

/* maps n to a pool index and allocates a block from it, returns NULL if the pool
has no more free-blocks, this function doesn't check if n maps to a invalid index */
void* poolset_pull_quick(poolset_t* set, size_t n);

/* recycle a block to its corresponding pool within the pool array in set
its valid to use the recycling functions found in pool.h to recycle blocks */
void poolset_push(poolset_t* set, void* payload);

#endif //POOLSET_ALLOCATOR_H__
