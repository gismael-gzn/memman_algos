#ifndef CELL_BLOCK_DEFINITION_H__
#define CELL_BLOCK_DEFINITION_H__

#include "extrastddef.h"
#include "align_config.h"

/* a cell is a structure that contains a block of metada and a set of
contiguous granules that represent an available payload.
Cells are defined in such way that allows them to be stored as free lists when
they are free, and to store an ownership pointer when they are allocated,
such pointer represents the cell owner, or "allocated from" relationship.
The granules member is the one returned by allocation functions,
as the returned pointer points to a location within the cell's variable,
then it's easy to recalculate the original cell address to reallocate, free or
retrieve any other information about the cell in nearly constant time.
This also means that writing outside the bounds of an allocation is forbidden.
 */

#if (GRANULE_SIZE_LOG2 < 3)
#undef GRANULE_SIZE_LOG2
#define GRANULE_SIZE_LOG2 (3)
#endif

#define granule_bytes (1<<(GRANULE_SIZE_LOG2))

#define owned_block                \
	union                          \
	{                              \
		SINGLY_ND(struct cell);    \
		void *owner;               \
		byte_t pad[granule_bytes]; \
	}
#define owned_block_alt            \
	union                          \
	{                              \
		struct cell *p_______;     \
		void *owner;               \
		byte_t pad[granule_bytes]; \
	}
#define cell_struct_members \
	owned_block;            \
	byte_t granules[granule_bytes];
#define cell_struct_members_alt \
	owned_block_alt;            \
	byte_t granules[granule_bytes];

#define cell_overhead_size (offsetof(cell, granules))

typedef struct cell
{
	cell_struct_members
}cell;

static inline void cell_set_owner(cell* c, void* owner)
{
	c->owner = owner;
}

static inline cell* canonic_ptr(void* payload)
{
	return (cell*)(byteptr(payload) - cell_overhead_size);
}

static inline void* canonic_ptr_head(void* payload)
{
	return (void*)(byteptr(payload) - cell_overhead_size);
}

// static inline int print_cell(cell* c)
// {
// 	return 
// 	fprintf(stdout, "owner: <%p> cell addr: <%p>, granules addr: <%p>\n", 
// 	c->owner, c, c->granules);
// }

#endif //CELL_BLOCK_DEFINITION_H__
