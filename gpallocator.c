#include "gpallocator.h"
#include "doubly.h"
#include "singly.h"
#include "cell.h"

typedef struct chunk_dnode
{
	DOUBLY_ND(struct chunk_dnode);
	size_t size;
	cell_struct_members_alt;
}chunk_dnode;

static inline chunk_dnode* chunk_canonic_ptr(byte_t* payload)
{
	return (chunk_dnode*)(payload - offsetof(chunk_dnode, granules));
}

static chunk_dnode* new_chunk_node(malloc_impl* mallochook, size_t size, void* owner)
{
	chunk_dnode* cn = mallochook(sizeof *cn + size - 1);
	*cn = (chunk_dnode){.size = size, {.owner = owner, }, };
	return cn;
}

static void realloc_chunk_node
(realloc_impl* reallochook, chunk_dnode** cn, size_t newsize)
{
	void* safe = reallochook(*cn, newsize);
	if(!isnull(safe))
		*cn = safe;
}


typedef struct pool_dnode
{
	owned_block_alt;
	DOUBLY_ND(struct pool_dnode);
	idpool_t* pool;
}pool_dnode;

static pool_dnode* new_pool_dnode
(malloc_impl* mallochook, size_t memsize, size_t segment, void* owner)
{
	pool_dnode* pdn = NULL;
	size_t totalmem = sizeof *pdn;
	totalmem += memsize + to_nearest_multiple(totalmem, 1<<GRANULE_SIZE_LOG2);

	byte_t* mem = mallochook(totalmem);
	pdn = (void*)mem;
	mem += sizeof *pdn, mem = align_pointer(mem, 1<<GRANULE_SIZE_LOG2);

	// memset(pdn, 0, totalmem);

	*pdn = (pool_dnode){
		.owner = owner, .pool = idpool_init(mem, memsize, segment, pdn),
	};

	return pdn;
}


def_doubly_list(pool_list, pool_dnode);

static inline void pool_list_add(pool_list* list, pool_dnode* nd)
{
	dl_insb(get_head(list), nd)
}

static inline void pool_list_pop(pool_dnode* nd)
{
	dl_pop(nd)
}


def_doubly_list(chunk_list, chunk_dnode);

static inline void chunk_list_add(chunk_list* list, chunk_dnode* nd)
{
	dl_insb(get_head(list), nd)
}

static inline void chunk_list_pop(chunk_dnode* nd)
{
	dl_pop(nd)
}

enum priority {in_preallocated, in_extra};

struct gpallocator_t
{
	owned_block;
	struct memman_hooks hooks;
	size_t poolsets, max_size;
	enum priority search;
	chunk_list big_chunks;
	pool_list extra_allocation;
	poolset_t *preallocated[];
};

static inline unsigned get_ownerships(void *save[const 5], void* start)
{
	unsigned block_type = 0;
	owned_block_alt *trav = !isnull(start)? canonic_ptr_head(start): NULL;

	save[block_type] = trav = trav->owner, ++block_type;
		save[block_type] = trav = trav->owner, ++block_type;
			save[block_type] = trav = trav->owner, ++block_type;
	save[block_type] = trav = trav->owner, ++block_type;

	// while(!isnull(trav))
	// 	save[block_type] = trav = trav->owner, ++block_type;

	return block_type-1;
}

/* 
block -> pool -> allocator
block -> pool -> node -> allocator
block -> allocator
*/

size_t gpallocated_size(void* payload)
{
	enum block_type {standalone=1, preallocated=3, extra=4};
	void *ownership_chain[5] = {0};
	unsigned type = get_ownerships(ownership_chain, payload);

	switch (type)
	{
	case standalone:
		return chunk_canonic_ptr(payload)->size;

	case extra:
	case preallocated:
		return payload_pool_segsize(payload);
	}

	return type;
}

static inline void sanitize_hooks(struct memman_hooks* hooks_ref)
{
	if(isnull(hooks_ref->mallochook))
		hooks_ref->mallochook = malloc;
	if(isnull(hooks_ref->reallochook))
		hooks_ref->reallochook = realloc;
	if(isnull(hooks_ref->freehook))
		hooks_ref->freehook = free;
}

gpallocator_t* gpallocator_new(struct memman_hooks hooks, size_t sets, 
size_t step, size_t max_block, void* id)
{
	sanitize_hooks(&hooks);
	gpallocator_t* ref = hooks.mallochook
	(sizeof *ref + sets*sizeof(poolset_t*));

	if(!isnull(ref))
	{
		*ref = (gpallocator_t){
			NULL,
			hooks,
			sets,
			max_block,
			in_preallocated,
			dl_compound(chunk_list, &ref->big_chunks, ),
			dl_compound(pool_list, &ref->extra_allocation, ),
		};

		if (sets > 0)
			for(size_t i=0; i<sets; ++i)
				ref->preallocated[i] = poolset_new
				(hooks.mallochook, step, max_block, ref);
	}

	return ref;
}

void gpallocator_del(gpallocator_t* ref)
{

}

static inline void* find_in_preallocated(gpallocator_t* ref, size_t n)
{
	void* payload = NULL;
	for(size_t i=0; i<ref->poolsets; ++i)
	{
		payload = poolset_pull(ref->preallocated[i], n);
		if(!isnull(payload))
		{
			poolset_t **a = ref->preallocated, **b=a+i, *sv = *a;
			*a = *b, *b = sv;
			ref->search = in_preallocated;
			break;
		}
	}
	return payload;
}

static inline void* find_in_extra(gpallocator_t* ref, size_t n)
{
	ref->search = in_extra;
	void* payload = NULL;
	pool_list* extra = &ref->extra_allocation;

	for(pool_dnode* i=get_head(extra); iauto(extra, i); i=get_next(i))
	{
		pool_t* check = to_pool_t(i->pool);
		if(!isnull(check))
		{
			if(n <= pool_segsize(check) && pool_capacity(check))
			{
				payload = pool_pull(check);
				if(get_head(extra) != i)
					pool_list_pop(i),
					pool_list_add(extra, i);
			}
			else
				continue;
		}
	}

	if(isnull(payload))
	{
		pool_dnode* fallback = new_pool_dnode
		(ref->hooks.mallochook, ref->max_size*8, n, ref);
		
		if(!isnull(fallback))
		{
			pool_t* as_pool = to_pool_t(fallback->pool);
			payload = pool_pull(as_pool);
			pool_list_add(extra, fallback);
		}
	}

	return payload;
}

void* gpallocator_malloc(gpallocator_t* ref, size_t n)
{
	if(n <= ref->max_size)
	{
		void* payload = NULL;
		switch (ref->search)
		{
		// case in_preallocated:
		// 	payload = find_in_preallocated(ref, n);

		default:
		case in_extra:
			if(isnull(payload))
				payload = find_in_extra(ref, n);
		}

		return payload;
	}

	else
	{
		chunk_dnode* chunk = new_chunk_node(ref->hooks.mallochook, n, ref);
		chunk_list_add(&ref->big_chunks, chunk);
		return chunk->granules;
	}

	return NULL;
}
