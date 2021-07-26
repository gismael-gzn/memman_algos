#include "poolset.h"
#include <stdio.h>

struct poolset_t
{
	size_t step, poolno;
	idpool_t *pool[];
};

size_t poolset_sizeof(size_t poolno)
{
	return sizeof(poolset_t) + poolno*sizeof(idpool_t*);
}

size_t poolset_smallestsize(poolset_t* chain)
{
	return chain->step;
}

size_t poolset_biggestsize(poolset_t* chain)
{
	return pool_segsize(to_pool_t(chain->pool[chain->poolno-1]));
}

// Check this one today
poolset_t *poolset_init(void *mem, size_t memsize, size_t poolmem, size_t step)
{
	poolset_t* set = NULL;
	// void* base = mem;
	if(!isnull(mem) && memsize >= sizeof *set)
	{
		set = mem;
		step += to_nearest_multiple(step, 1<<GRANULE_SIZE_LOG2);

		memsize -= sizeof *set;

		size_t poolno = memsize/poolmem, 
		pool_arr_size = poolno*sizeof(idpool_t*);
		if(memsize >= pool_arr_size)
		{
			byte_t* memptr = byteptr(mem) + poolset_sizeof(poolno);
			memptr = align_pointer(memptr, 1<<GRANULE_SIZE_LOG2);
			memsize -= pool_arr_size;
			memsize = ptroff(memptr, memptr+memsize);

			size_t i=0;
			for(; (i+1)*poolmem<=memsize; memptr += poolmem, ++i)
			{
				size_t segsize = (i+1)*step;
				if(segsize >= poolmem - idpool_sizeof())
					poolmem += step;
				set->pool[i] = idpool_init(memptr, poolmem, segsize, set);

				pool_t* upcast = to_pool_t(set->pool[i]);
				printf(">>%zu::%zu\n", i, pool_segsize(upcast));
			}
			*set = (poolset_t){step, i, };
		}
	}
	return set;
}

poolset_t* poolset_new(malloc_impl mallochook, size_t memsize, size_t poolmem, size_t step)
{
	if(isnull(mallochook))
		mallochook = malloc;
	return poolset_init(mallochook(memsize), memsize, poolmem, step);
}

void* poolset_pull(poolset_t* set, size_t n)
{
	size_t idx = (n+!n-1)/set->step;
	printf("mapping %zu/%zu\n", idx, set->poolno);
	return idx < set->poolno? pool_pull(to_pool_t(set->pool[idx])): NULL;
}

void* poolset_pull_quick(poolset_t* set, size_t n)
{
	size_t idx = (n+!n-1)/set->step;
	return pool_pull(to_pool_t(set->pool[idx]));
}

void* poolset_push(poolset_t* set, void* payload)
{
	size_t n = payload_pool_segsize(payload), idx = (n+!n-1)/set->step;
	pool_push(to_pool_t(set->pool[idx]), payload);
}
