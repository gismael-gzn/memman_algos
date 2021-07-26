#include "gpallocator.h"
#include "doubly.h"
#include "singly.h"

typedef struct chunk_dnode
{
	DOUBLY_ND(struct chunk_dnode);
	size_t size;
	void* owner;
	byte_t payload[1];
}chunk_dnode;

static inline chunk_dnode* chunk_canonic_ptr(byte_t* payload)
{
	return (chunk_dnode*)(payload - offsetof(chunk_dnode, payload));
}

static chunk_dnode* new_chunk_node(malloc_impl* mallochook, size_t size, void* owner)
{
	chunk_dnode* cn = mallochook(sizeof *cn + size - 1);
	*cn = (chunk_dnode){.size = size, .owner = owner, };
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

	pdn->pool = idpool_init(mem, memsize, segment, owner);

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
	void* owner;
	struct memman_hooks hooks;
	size_t poolsets, max_size;
	enum priority search;
	chunk_list big_chunks;
	pool_list extra_allocation;
	poolset_t *preallocated[];
};

static inline void* find_in_preallocated(gpallocator_t* allocator, size_t n)
{
	void* payload = NULL;
	for(size_t i=0; i<allocator->poolsets; ++i)
	{
		assert(allocator->preallocated[i]);
		payload = poolset_pull(allocator->preallocated[i], n);
		if(!isnull(payload))
		{
			poolset_t **a = allocator->preallocated, **b=a+i, *sv = *a;
			*a = *b, *b = sv;
			allocator->search = in_preallocated;
			break;
		}
	}
	return payload;
}

static inline void* find_in_extra(gpallocator_t* alloc, size_t n)
{
	void* payload = NULL;

	pool_list* extra = &alloc->extra_allocation;
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
		(alloc->hooks.mallochook, alloc->max_size+idpool_sizeof()+cell_overhead_sizeof(), n, alloc);
		if(!isnull(fallback))
		{
			pool_t* as_pool = to_pool_t(fallback->pool);
			payload = pool_pull(as_pool);
			pool_list_add(extra, fallback);
		}
	}

	alloc->search = in_extra;

	return payload;
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

gpallocator_t* gpallocator_new(struct memman_hooks hooks, size_t poolset_chunk,
size_t initial_poolsets, size_t pool_chunk, size_t step, void* id)
{
	sanitize_hooks(&hooks);
	gpallocator_t* allocator = hooks.mallochook
	(sizeof *allocator + initial_poolsets*sizeof(poolset_t*));

	if(!isnull(allocator))
	{
		*allocator = (gpallocator_t){
			id,
			hooks,
			initial_poolsets,
			0,
			in_preallocated,
			dl_compound(chunk_list, &allocator->big_chunks, ),
			dl_compound(pool_list, &allocator->extra_allocation, ),
		};

		if (initial_poolsets > 0)
			for(; initial_poolsets; --initial_poolsets)
			{
				allocator->preallocated[initial_poolsets-1] =
				poolset_new(hooks.mallochook, poolset_chunk, pool_chunk, step);
			}

		printf("%p\n", *allocator->preallocated);
		allocator->max_size = poolset_biggestsize(*allocator->preallocated);
	}

	return allocator;
}

void gpallocator_del(gpallocator_t* allocator)
{

}

void* gpallocator_malloc(gpallocator_t* allocator, size_t n)
{
	if(n <= allocator->max_size)
	{
		void* payload = NULL;
		switch (allocator->search)
		{
		case in_preallocated:
			printf("max size is %zu\n", allocator->max_size);
			payload = find_in_preallocated(allocator, n);

		case in_extra:
			if (isnull(payload))
			{
				payload = find_in_extra(allocator, n);
			}
		}

		return payload;
	}

	else
	{
		chunk_dnode* chunk = new_chunk_node
		(allocator->hooks.mallochook, n, allocator);
		chunk_list_add(&allocator->big_chunks, chunk);
		return chunk->payload;
	}

	return NULL;
}
