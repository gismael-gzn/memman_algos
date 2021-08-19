#include "gpallocator.h"
#include "doubly.h"
#include "cell.h"

typedef struct chunk_dnode chunk_dnode;

/* To ensure that the payload has proper alignment.
Might as well just use _Alignas(granule_bytes) */
#if (GRANULE_SIZE_LOG2 > 3)
typedef struct chunk_dnode_head__
{
	DOUBLY_ND(chunk_dnode);
	size_t size;
}chunk_dnode_head__;
#define padding_for_large_allocations \
	byte_t padding                    \
		[to_nearest_multiple(offsetof(chunk_dnode_head__, size), granule_bytes)];
#else
#define padding_for_large_allocations 
#endif

struct chunk_dnode
{
	padding_for_large_allocations
	DOUBLY_ND(struct chunk_dnode);
	size_t size;
	cell_struct_members_alt
};

#define large_block_overhead offsetof(chunk_dnode, granules)

static inline chunk_dnode* chunk_canonic_ptr(void* payload)
{
	return (chunk_dnode*)(byteptr(payload) - offsetof(chunk_dnode, granules));
}

static inline chunk_dnode* new_chunk_node
(malloc_impl* mallochook, size_t size, void* owner)
{
	chunk_dnode* cn = mallochook(size + offsetof(chunk_dnode, granules));
	*cn = (chunk_dnode){.size = size, {.owner = owner, }, };
	return cn;
}

// null guard might go here
static inline void realloc_chunk_node
(realloc_impl* reallochook, chunk_dnode** cn, size_t newsize)
{
	void* safe = reallochook(*cn, newsize + large_block_overhead);
	if(!isnull(safe))
		*cn = safe,
		(*cn)->size = newsize;
}


typedef struct pool_dnode
{
	owned_block_alt;
	DOUBLY_ND(struct pool_dnode);
	pool_t* pool;
}pool_dnode;

static inline pool_dnode* new_pool_dnode
(malloc_impl* mallochook, size_t memsize, size_t segment, void* owner)
{
	pool_dnode* pdn = NULL;
	size_t totalmem = sizeof *pdn;
	totalmem += memsize + to_nearest_multiple(totalmem, 1<<GRANULE_SIZE_LOG2);

	byte_t* mem = mallochook(totalmem);
	pdn = (void*)mem;
	mem += sizeof *pdn, mem = align_pointer(mem, 1<<GRANULE_SIZE_LOG2);

	*pdn = (pool_dnode){
		.owner = owner, .pool = pool_init(mem, memsize, segment, pdn),
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

static inline void sanitize_hooks(struct memman_hooks* hooks_ref)
{
	if(isnull(hooks_ref->mallochook))
		hooks_ref->mallochook = malloc;
	if(isnull(hooks_ref->reallochook))
		hooks_ref->reallochook = realloc;
	if(isnull(hooks_ref->freehook))
		hooks_ref->freehook = free;
}

enum priority {in_preallocated, in_extra};

struct gpallocator_t
{
	owned_block;
	struct memman_hooks hooks;
	size_t poolset_no, min_size, max_size;
	enum priority search;
	chunk_list big_allocs;
	pool_list on_request;
	poolset_t *initial[];
};

// mod
static inline void* find_in_preallocated(gpallocator_t* ref, size_t n)
{
	void* payload = NULL;
	for(size_t i=0; i<ref->poolset_no; ++i)
	{
		payload = poolset_pull(ref->initial[i], n);
		if(!isnull(payload))
		{
			if(i != 0)
			{
				poolset_t **a = ref->initial, **b=a+i, *sv = *a;
				*a = *b, *b = sv;
			}
			ref->search = in_preallocated;
			break;
		}
	}
	return payload;
}

// mod
static inline void* find_in_extra(gpallocator_t* ref, size_t n)
{
	ref->search = in_extra;
	void* payload = NULL;
	pool_list* extra = &ref->on_request;
	n += to_nearest_multiple(n, ref->min_size);

	for(pool_dnode* i=get_head(extra); iauto(extra, i); i=get_next(i))
	{
		pool_t* pool = i->pool;
		if(!isnull(pool))
		{
			if(n == pool_segsize(pool) && pool_available(pool))
			{
				payload = pool_pull(pool);
				if(get_head(extra) != i)
					pool_list_pop(i),
					pool_list_add(extra, i);
				break;
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
			pool_t* pool = fallback->pool;
			payload = pool_pull(pool);
			pool_list_add(extra, fallback);
		}
	}
	return payload;
}

typedef enum alloc_type
{
	large_ = 2, initial_ = 3, extra_ = 4,
}alloc_type;

/* block -> pool -> allocator
block -> pool -> node -> allocator
block -> allocator */
static inline alloc_type get_ownerships(void *save[const 4], void* start)
{
	alloc_type t = 0;
	owned_block_alt *trav = !isnull(start)? canonic_ptr_head(start): NULL;

	while(!isnull(trav))
		save[t] = trav = trav->owner, ++t;

	// for(size_t i=0; i<4; ++i)
	// 	printf("<%p> T:%d\n", save[i], t);

	return t;
}

static inline void *allocate_from_pool(gpallocator_t *ref, size_t n)
{
	void *payload = NULL;
	if(n <= ref->max_size)
		switch (ref->search)
		{
			case in_preallocated:
				payload = find_in_preallocated(ref, n);

			if(isnull(payload))
			{
				default:
				case in_extra:
					payload = find_in_extra(ref, n);
			}
		}
	return payload;
}

static inline void *allocate_in_chunk(gpallocator_t *ref, size_t n)
{
	chunk_dnode* chunk = new_chunk_node(ref->hooks.mallochook, n, ref);
	chunk_list_add(&ref->big_allocs, chunk);
	return chunk->granules;
}

static inline size_t allocation_size
(void *ownerlist[const 4], alloc_type t, void *ptr)
{
	size_t retval = t;
	switch (t)
	{
		case large_:
			retval = chunk_canonic_ptr(ptr)->size;
		break;

		case extra_: case initial_:
		{
			pool_t *p = ownerlist[0];
			retval = pool_segsize(p);
		}
		break;
	}
	return retval;
}

static inline void deallocate_large(gpallocator_t *ref, void *payload)
{
	chunk_dnode *to_free = chunk_canonic_ptr(payload);
	chunk_list_pop(to_free);
	ref->hooks.freehook(to_free);
}

static inline void deallocate_extra
(gpallocator_t *ref, void *ownerlist[const 4], void *payload)
{
	pool_dnode *pdn = ownerlist[1];
	pool_t* p = pdn->pool;
	pool_push(p, payload);
	if(pool_freeable(p))
		pool_list_pop(pdn),
		ref->hooks.freehook(pdn);
}

static inline void deallocate_initial(void *ownerlist[const 4], void *payload)
{
	pool_t* p = ownerlist[0];
	pool_push(p, payload);
}

static inline void allocation_free
(void *ownerlist[const 4], alloc_type t, void *payload)
{
	gpallocator_t *owner = ownerlist[t-2];
	switch (t)
	{
		case large_: deallocate_large(owner, payload); break;
		case extra_: deallocate_extra(owner, ownerlist, payload); break;
		case initial_: deallocate_initial(ownerlist, payload); break;
	}
}

size_t gpallocated_size(void* payload)
{
	void *ownerlist[4] = {0};
	alloc_type t = get_ownerships(ownerlist, payload);
	return allocation_size(ownerlist, t, payload);
}

gpallocator_t* gpallocator_new(struct memman_hooks hooks, size_t sets, 
size_t step, size_t max_block)
{
	sanitize_hooks(&hooks);
	gpallocator_t* ref = hooks.mallochook
	(sizeof *ref + sets*sizeof(poolset_t*));

	if(!isnull(ref))
	{
		*ref = (gpallocator_t){
			NULL, hooks, sets, step, max_block, in_preallocated,
			dl_compound(chunk_list, &ref->big_allocs, ),
			dl_compound(pool_list, &ref->on_request, ),
		};

		if (sets > 0)
			for(size_t i=0; i<sets; ++i)
				ref->initial[i] = poolset_new
				(hooks.mallochook, step, max_block, ref);

	}

	return ref;
}

void gpallocator_del(gpallocator_t* ref)
{
	for(chunk_dnode *i=get_head(&ref->big_allocs), *f=NULL; 
	!lempty(&ref->big_allocs);)
		f = i, mv_next(i), chunk_list_pop(f), ref->hooks.freehook(f);

	for(pool_dnode *i=get_head(&ref->on_request), *f=NULL;
	!lempty(&ref->on_request); )
		f = i, mv_next(i), pool_list_pop(f), ref->hooks.freehook(f);

	for(size_t i=0; i<ref->poolset_no; ++i)
		ref->hooks.freehook(ref->initial[i]);

	ref->hooks.freehook(ref);
}


void* gpallocator_malloc(gpallocator_t* ref, size_t n)
{
	return n <= ref->max_size? allocate_from_pool(ref, n): allocate_in_chunk(ref, n);
}

// mod
void* gpallocator_realloc(gpallocator_t* ref, void* ptr, size_t n)
{
	if(isnull(ptr))
		return gpallocator_malloc(ref, n);

	void *ownerlist[4] = {0};
	alloc_type t = get_ownerships(ownerlist, ptr);
	size_t size = allocation_size(ownerlist, t, ptr),
	treshold = ref->min_size;

	if (n == 0)
	{
		allocation_free(ownerlist, t, ptr);
		return NULL;
	}

	void *payload = NULL;
	switch (t)
	{
	case large_:
		if (n <= ref->max_size)
			payload = allocate_from_pool(ref, n),
			memcpy(payload, ptr, payload_pool_segsize(payload)),
			deallocate_large(ref, ptr);
		else
		{
			chunk_dnode *c = chunk_canonic_ptr(ptr);
			chunk_list_pop(c);
			realloc_chunk_node(ref->hooks.reallochook, &c, n);
			chunk_list_add(&ref->big_allocs, c);
			payload = c->granules;
		}
	break;

	case initial_:
		if(n > ref->max_size)
			payload = allocate_in_chunk(ref, n);
		else if(n <= size-treshold || n > size)
			payload = allocate_from_pool(ref, n);
		else
		{
			payload = ptr;
			break;
		}
		memcpy(payload, ptr, n),
		deallocate_initial(ownerlist, ptr);
		break;

	case extra_:
		if(n > ref->max_size)
			payload = allocate_in_chunk(ref, n);
		else if(n <= size-treshold || n > size)
			payload = allocate_from_pool(ref, n);
		else
		{
			payload = ptr;
			break;
		}
		memcpy(payload, ptr, allocation_size(ownerlist, t, ptr));
		deallocate_extra(ref, ownerlist, ptr);
		break;
	}

	return payload;
}

void gpallocator_free(void* payload)
{
	if(isnull(payload))
		return;
	void *ownerlist[4] = {0};
	alloc_type t = get_ownerships(ownerlist, payload);
	allocation_free(ownerlist, t, payload);
}

static inline unsigned guard_null(void *ptr1, void *ptr2)
{
	return isnull(ptr1) || isnull(ptr2);
}

size_t gpptr_write(void *dst, void *src, size_t wrt_size)
{
	if(guard_null(dst, src)) return 0;
	size_t allowed_write = gpallocated_size(dst);
	if(wrt_size > allowed_write)
		wrt_size = allowed_write;
	memmove(dst, src, wrt_size);
	return wrt_size;
}

size_t gpptr_read(void *rd, void *buf, size_t rd_size)
{
	if(guard_null(rd, buf)) return 0;
	size_t allowed_read = gpallocated_size(rd);
	if(rd_size > allowed_read)
		rd_size = allowed_read;
	memmove(buf, rd, rd_size);
	return rd_size;
}

size_t gpptr_objwrite(void *to, void *from)
{
	return gpptr_read(from, to, gpallocated_size(to));
}
