#include "pool.h"
#include "stack.h"

#include "cell.h"

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
	owned_block;
	arena_t* chunk;
	size_t segsize;
	cells free_list;
};

size_t pool_sizeof(void)
{
	return sizeof(pool_t) + arena_sizeof();
}

size_t cell_overhead_sizeof(void)
{
	return cell_overhead_size;
}

size_t pool_available(pool_t* pool)
{
	return get_len(&pool->free_list)+arena_capacity(pool->chunk)/pool->segsize;
}

unsigned pool_freeable(pool_t* pool)
{
	return pool_available(pool) == arena_size(pool->chunk)/pool->segsize;
}

size_t pool_segsize(pool_t* pool)
{
	return pool->segsize - cell_overhead_size;
}

pool_t* pool_new(malloc_impl* mallochook, size_t mem_size, size_t segsize, void* owner)
{
	if(isnull(mallochook))
		mallochook = malloc;
	void* mem = mallochook(mem_size);
	return pool_init(mem, mem_size, segsize, owner);
}

pool_t* pool_init(void* mem, size_t mem_size, size_t segsize, void* owner)
{
	pool_t* pool = NULL;
	arena_t* arena = NULL;

	if(!isnull(mem))
	{
		pool = mem;
		size_t used_memory = sizeof *pool;
		used_memory += to_nearest_multiple(used_memory, granule_bytes);

		arena = arena_init
		((byteptr(mem)+used_memory), mem_size-used_memory, granule_bytes);
		if(isnull(arena))
			return NULL;

		if(segsize <= granule_bytes)
			segsize = sizeof(cell);
		else 
			segsize = segsize + cell_overhead_size,
			segsize += to_nearest_multiple(segsize, granule_bytes);

		*pool = (pool_t){
			owner, arena, segsize, sc_compound(cells, &pool->free_list, ),
			};
	}

	return pool;
}

void* pool_pull(pool_t* pool)
{
	void* payload = NULL;
	cell* new_cell = NULL;

	// printf("capacity: %zu, need: %zu\n", 
	// 	arena_capacity(pool->chunk), pool->segsize);

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
	return n <= pool_segsize(pool)? pool_pull(pool): NULL;
}

void pool_push(pool_t* pool, void* payload)
{
	cell* free_cell = canonic_ptr(payload);
	cells_add(&pool->free_list, free_cell);
}

extern inline size_t payload_pool_segsize(void* payload)
{
	cell* c = canonic_ptr(payload);
	return pool_segsize((pool_t*)c->owner);
}

pool_t* payload_owner_pool(void* payload)
{
	cell* c = canonic_ptr(payload);
	return c->owner;
}

void payload_pushto_pool(void* payload)
{
	pool_push(canonic_ptr(payload)->owner, payload);
}
