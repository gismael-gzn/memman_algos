#ifndef GENERAL_PURPOSE_ALLOCATOR_H__
#define GENERAL_PURPOSE_ALLOCATOR_H__

/* General purpose allocator.
Functional allocator with a pre-allocated set of poolsets for 'small' blocks,
at-request allocation for large blocks and for when all the poolsets get emptied,
*/

#include "poolset.h"
#include <string.h>
#include <stdio.h>

/* memory management function pointers */
struct memman_hooks
{
	malloc_impl* mallochook;
	realloc_impl* reallochook;
	free_impl* freehook;
};

/* gpallocator opaque type */
typedef struct gpallocator_t gpallocator_t;

/* 
Create a new allocator which manages its memory regions using the memman hooks,
if a hook is NULL, then said pointer gets re-assigned to its stdlib homolog.
'sets' is the number of pre-allocated poolsets for the allocator, these poolsets
last for the whole program, or until the allocator gets destroyed,
'step' and 'max_block' serve the same purpose as in poolset initializer function,
in this case 'max_block' also tells the allocator that any request of size > max_block
will get a large-block allocation.
 */
gpallocator_t* gpallocator_new(struct memman_hooks hooks, size_t sets, 
size_t step, size_t max_block);

/* destroy an allocator using its memman hooks */
void gpallocator_del(gpallocator_t* allocator);

/* request an allocation large enough to fit n bytes
if n is 0, the allocator treats it as a request of size 1 */
void* gpallocator_malloc(gpallocator_t* allocator, size_t n);

/* request to reallocate ptr so it has an effective payload of at least n bytes,
if ptr is NULL then gpallocator gets called, if n is 0 and ptr is not NULL,
then the block to which ptr points to gets freed */
void* gpallocator_realloc(gpallocator_t* ref, void* ptr, size_t n);

/* if the block pointed to by payload is owned by a preallocated poolset,
then the block gets recycled. If the block belongs to a post-allocated pool,
then it gets recycled, if the owner pool gets empty when recycling said block,
then that pool gets freed using the allocator owner hook.
Lastly, if payload points to a large block, this gets insta-freed.
Passing NULL to this functions makes the function to perform no action.  */
void gpallocator_free(void* payload);

/* get the effective size of a block */
size_t gpallocated_size(void* payload);

/* safely write over the effective payload of dst from src, 
returns the number of effectively writen bytes, the size of the space pointed
to by src must be at least that of wrt_size 
src may be overlapped with the payload of dst */
size_t gpptr_write(void *dst, void *src, size_t wrt_size);

/* safely write the contents of the effective payload pointed to by ptr into buf,
returns the number of effectively written bytes to buf, the size of the space
pointed to by buf must be at least that of buf_size,
buf may be overlapped with the payload of ptr*/
size_t gpptr_read(void *ptr, void *buf, size_t buf_size);

/* being 'to' and 'from' pointers to an effective payload,
write at most gpallocated_size(to) bytes to 'to' from 'from' 
returns the number of effectively writen bytes, the only way that to and from 
may overlap is if to == from */
size_t gpptr_objwrite(void *to, void *from);

#endif //GENERAL_PURPOSE_ALLOCATOR_H__
