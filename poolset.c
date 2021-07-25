#include "poolset.h"
#include <stdio.h>

struct poolset_t
{
	size_t step;
	struct
	{
		size_t elements;
		idpool_t *pool[];
	};
};


size_t poolchain_memsize(size_t poolno)
{
	return sizeof(poolset_t) + poolno*sizeof(idpool_t*);
}


poolset_t *poolset_init(void *mem, size_t memsize, size_t poolmem, size_t step)
{
	poolset_t* set = NULL;
	void* base = mem;
	if(!isnull(mem) && memsize >= sizeof *set)
	{
		set = mem;
		step += to_nearest_multiple(step, 1<<GRANULE_SIZE_LOG2);

		memsize -= sizeof *set;

		size_t pool_arr_size = memsize/poolmem*sizeof(idpool_t*);
		if(memsize >= pool_arr_size)
		{
			byte_t* memptr = mem;
			memsize -= pool_arr_size;
			memptr += pool_arr_size;
			memptr = align_pointer(memptr, 1<<GRANULE_SIZE_LOG2);
			memsize = ptroff(memptr, memptr+memsize);

			for (size_t i=0; (i+1)*poolmem<=memsize; memptr += poolmem, ++i)
			{
				set->pool[i] = idpool_init(memptr, poolmem, (i+1)*step, set);
			}
		}
	}

	return set;
}
