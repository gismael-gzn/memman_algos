#ifndef POOL_ALLCOATOR_H__
#define POOL_ALLCOATOR_H__

#include "align_config.h"
#include "arena.h"

/* Pool based / fixed-size block allocator
Allocate a large enough memory region to then sub-allocate a finite quantity of 
fixed-size blocks.
pools are freeable by using the inverse of the malloc-like function from which 
the pool variable got its memory (check the initializer functions).
For example if malloc is used, then to free the pool simple use free(pool) */


/* pool opaque type */
typedef struct pool_t pool_t;

/* Might be unused as for now
returns the number of bytes before the granules member of a cell */
size_t cell_overhead_sizeof(void);

/* Might be unused as for now
returns the size of the pool_t complete type */
size_t pool_sizeof(void);

/* returns the effective segment size of a pool,
that is, the bytes per block allocation */
size_t pool_segsize(pool_t* pool);

/* returns the number of blocks available in the pool */
size_t pool_available(pool_t* pool);

/* returns 1 if all the blocks in the pool are free, otherwise returns 0 */
unsigned pool_freeable(pool_t* pool);

/* Initializes a pool with a preallocated block of memory of size mem_size,
if mem_size doesn't suffice to allocate the pool + an arena allocator, NULL is returned,
it configures the pool so that each allocation has an effective payload of segsize bytes.
If segsize is not a multiple of the defined size for granules, then it's adjusted
to be so, re-adjustment is not performed if segsize is a multiple of a granule's size.
Owner represents the owner of the pool, check cell.h for more info */
pool_t* pool_init(void* mem, size_t mem_size, size_t segsize, void* owner);

/* Same as above, but instead of taking a preallocated block, it takes a pointer
to  a malloc-like function to call with the argument 'mem_size'.
If mallochook is NULL, then stdlib's malloc is used */
pool_t* pool_new(malloc_impl* mallochook, size_t mem_size, size_t segment, void* owner);

/* allocate a block from pool, returns NULL if there are no blocks available */
void* pool_pull(pool_t* pool);

/* allocate a block from pool, it returns NULL if there are no blocks available
or n is bigger than the pool's segment size */
void* pool_pop_check(pool_t* pool, size_t n);

/* passing a payload pointer that wasn't allocated by a pool might cause errors
such as, stack/heap buffer overflow, heap corruption, etc.  */

/* free a block to a pool, that is, recyling it to the pool's free list  */
void pool_push(pool_t* pool, void* payload);

/* check cell.h for more info about the next 3 functions. */

/* given a pointer pointing to a previously pool-allocated segment, 
returns the segment_size of the pool to which said block belongs */
size_t payload_pool_segsize(void* payload);

/* return the pool to which a previously pool-allocated block pointed to by payload */
pool_t* payload_owner_pool(void* payload);

/* recycle a previously pool-allocated block to which payload points to */
void payload_pushto_pool(void* payload);

#endif //POOL_ALLCOATOR_H__
