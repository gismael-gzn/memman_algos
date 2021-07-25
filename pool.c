#include "pool.h"
#include "stack.h"
#include <stdio.h>

#if (GRANULE_SIZE_LOG2 < 3)
#define granule_bytes (8ul)
#else
#define granule_bytes (1<<(GRANULE_SIZE_LOG2))
#endif

#if defined(MEM_ALGOS_SHOULD_SUPPORT_STDALIGN)
#include <stdalign.h>
#define cell_struct_members     \
	union                       \
	{                           \
		SINGLY_ND(struct cell); \
		void *owner;            \
	};                          \
	alignas(granule_bytes) byte_t granules[granule_bytes];
#else
#define cell_struct_members        \
	union                          \
	{                              \
		SINGLY_ND(struct cell);    \
		void *owner;               \
		byte_t pad[granule_bytes]; \
	};                             \
	byte_t granules[granule_bytes];
#endif

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

static inline int print_cell(cell* c)
{
	return 
	fprintf(stdout, "owner: <%p> cell addr: <%p>, granules addr: <%p>\n", 
	c->owner, c, c->granules);
}


def_stack_container(cells, cell);

static inline void cells_add(cells* list, cell* nd)
{
	sc_add(list, nd)
}

static inline cell* cells_pop(cells* list)
{
	cell* save = NULL;
	sc_get_pop(list, save);
	return save;
}

struct pool_t
{
	arena_t* chunk;
	size_t segsize;
	cells free_list;
};

size_t pool_capacity(pool_t* pool)
{
	return get_len(&pool->free_list)+arena_capacity(pool->chunk)/pool->segsize;
}

size_t pool_segsize(pool_t* pool)
{
	return pool->segsize - cell_overhead_size;
}

pool_t* pool_init(void* mem, size_t mem_size, size_t segsize)
{
	pool_t* pool = NULL;

	if(!isnull(mem))
	{
		arena_t* arena = NULL;
		pool = mem;
		arena = arena_init
		((byteptr(mem)+sizeof *pool), mem_size-sizeof *pool, granule_bytes);

		if(segsize <= granule_bytes)
			segsize = sizeof(cell);
		else 
			segsize = segsize + cell_overhead_size,
			segsize += to_nearest_multiple(segsize, granule_bytes);

		*pool = (pool_t){
			arena, segsize, sc_compound(cells, &pool->free_list, )
			};
	}

	return pool;
}

pool_t* pool_new(malloc_impl* mallochook, size_t mem_size, size_t segsize)
{
	if(isnull(mallochook))
		mallochook = malloc;
	void* mem = mallochook(mem_size);
	return pool_init(mem, mem_size, segsize);
}

void* pool_pop(pool_t* pool)
{
	void* payload = NULL;
	cell* new_cell = NULL;

	if(pool->segsize <= arena_capacity(pool->chunk))
		new_cell = arena_malloc_quick(pool->chunk, pool->segsize),
		cell_set_owner(new_cell, pool),
		payload = new_cell->granules;
	else if(get_len(&pool->free_list))
		new_cell = cells_pop(&pool->free_list),
		cell_set_owner(new_cell, pool),
		payload = new_cell->granules;

	return payload;
}

void* pool_pop_check(pool_t* pool, size_t n)
{
	return n <= pool_segsize(pool)? pool_pop(pool): NULL;
}

void pool_push(pool_t* pool, void* payload)
{
	cell* free_cell = canonic_ptr(payload);
	cells_add(&pool->free_list, free_cell);
}

extern inline size_t payload_size(void* payload)
{
	cell* c = canonic_ptr(payload);
	return pool_segsize((pool_t*)c->owner);
}

void* payload_owner_pool(void* payload)
{
	cell* c = canonic_ptr(payload);
	return c->owner;
}

void payload_pushto_owner(void* payload)
{
	pool_push(canonic_ptr(payload)->owner, payload);
}
