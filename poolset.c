#include "poolset.h"

struct poolset_t
{
	size_t step, poolno;
	pool_t *pool[];
};

size_t poolset_sizeof(size_t poolno)
{
	return sizeof(poolset_t) + poolno*sizeof(pool_t*);
}

size_t poolset_smallestsize(poolset_t* set)
{
	return set->step;
}

size_t poolset_biggestsize(poolset_t* set)
{
	return pool_segsize(set->pool[set->poolno-1]);
}

poolset_t* poolset_new(malloc_impl* mallochook, size_t step, size_t max_block, void* owner)
{
	if(isnull(mallochook))
		mallochook = malloc;

	step += to_nearest_multiple(step, 1<<GRANULE_SIZE_LOG2);
	max_block += to_nearest_multiple(max_block, step);

	size_t pools = max_block/step, 
	poolmem = max_block*8,
	overhead_size = poolset_sizeof(pools);

	overhead_size += to_nearest_multiple(overhead_size, step);
	poolmem += to_nearest_multiple(poolmem, step);

	poolset_t* set = mallochook(overhead_size + pools*poolmem);

	if(!isnull(set))
	{
		byte_t* mem = byteptr(set);
		mem += overhead_size;

		*set = (poolset_t){ step, pools, };

		size_t i=0;
		for(; i<pools; ++i, mem += poolmem)
		{
			set->pool[i] = pool_init(mem, poolmem, (i+1)*step, owner);
			// pool_t* upcast = (set->pool[i]);
			// printf("poolmem: %zu segsize: %zu\n", poolmem, max_block*4);
			// printf(">>%zu::%zu\n", i, pool_segsize(upcast));
		}
	}

	return set;
}

void* poolset_pull(poolset_t* set, size_t n)
{
	size_t idx = (n+!n-1)/set->step;
	// printf("mapping %zu/%zu\n", idx, set->poolno-1);
	return idx < set->poolno? pool_pull(set->pool[idx]): NULL;
}

void* poolset_pull_quick(poolset_t* set, size_t n)
{
	size_t idx = (n+!n-1)/set->step;
	return pool_pull(set->pool[idx]);
}

void poolset_push(poolset_t* set, void* payload)
{
	size_t n = payload_pool_segsize(payload), idx = (n+!n-1)/set->step;
	pool_push(set->pool[idx], payload);
}
