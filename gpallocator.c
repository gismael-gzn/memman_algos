#include "gpallocator.h"
#include "doubly.h"
#include "singly.h"

typedef struct chunk_node
{
	DOUBLY_ND(struct chunk_node);
	size_t size;
	byte_t payload[1];
};

typedef struct pool_dnode
{
	DOUBLY_ND(struct pool_dnode);
	idpool_t* pool;
}pool_dnode;

typedef struct poolset_snode
{
	SINGLY_ND(struct poolset_snode);
	poolset_t* set;
}poolset_snode;

def_doubly_list(pool_list, pool_dnode);

def_singly_list(poolset_list, poolset_snode);

struct gpallocator_t
{
	size_t max_pool_segsize;
	malloc_impl* mallochook;
	realloc_impl* reallochook;
	free_impl* freehook;
	poolset_list constant;
	pool_list extra;
};
