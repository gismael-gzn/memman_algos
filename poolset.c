#include "poolset.h"
#include <stdio.h>


typedef struct owned_pool
{
	poolchain_t* owner;
	pool_t* pool;
}owned_pool;



struct poolchain_t
{
	size_t step;
	struct
	{
		size_t collection_size;
		pool_t *collection[];
	};
};


size_t poolchain_memsize(size_t poolno)
{
	return sizeof(poolchain_t) + poolno*sizeof(pool_t*);
}

poolchain_t *poolchain_init(void *mem, size_t memsize, size_t pool_bytes,
							size_t step);

poolchain_t *poolchain_init(void *mem, size_t memsize, size_t pool_bytes,
							size_t step)
{
	poolchain_t* chain = NULL;
	void* base = mem;
	if(!isnull(mem) && memsize >= sizeof *chain)
	{
		chain = mem;
		step += to_nearest_multiple(step, 1<<GRANULE_SIZE_LOG2);

		memsize -= sizeof *chain;


		size_t pool_arr_size = sizeof(pool_t*)*memsize/pool_bytes;
		if(memsize >= pool_arr_size)
		{
			memsize -= pool_arr_size;
			mem = byteptr(mem) + poolchain_memsize(memsize/pool_bytes);
			mem = align_pointer(mem, 1 << GRANULE_SIZE_LOG2);

			for (size_t ctrl=0, i=0; ctrl+pool_bytes<=memsize; ctrl += pool_bytes, 
				mem = byteptr(mem) + pool_bytes, ++i)
			{
				chain->collection[i] = pool_init(mem, pool_bytes, i*step);
				printf("%zu\n", ptrdiff(base, mem));
			}
		}
	}

	return chain;
}
