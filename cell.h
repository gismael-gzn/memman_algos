#ifndef OWNED_BLOCK_DEFINITION_H__
#define OWNED_BLOCK_DEFINITION_H__

#include "extrastddef.h"
#include "align_config.h"

#if (GRANULE_SIZE_LOG2 < 3)
#define granule_bytes (8ul)
#else
#define granule_bytes (1<<(GRANULE_SIZE_LOG2))
#endif

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

#endif //OWNED_BLOCK_DEFINITION_H__
