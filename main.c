#include "gpallocator.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

void unit_test1(int argc, char const *argv[])
{
	const char* info = 
	"this unit test consists in checking if all the allocations are being made"
	"with a correct offset, so there are no invalid reads/writes and it's"
	"possible to free all the blocks";

	pool_t* p = pool_new(malloc, 4096, 0);
	size_t m = pool_capacity(p);
	char* mem[m];

	for(size_t i=0; i<m; ++i)
	{
		mem[i] = pool_pull(p);
		memset(mem[i], 7777, payload_pool_segsize(mem[i]));
		assert(payload_owner_pool(mem[i]) == p);
		assert((uintptr_t)mem[i]%8==0);
	}
	printf("pool capacity: %zu\n", pool_capacity(p));

	for(size_t i=0; i<m; ++i)
		payload_pushto_pool(mem[i]);
	printf("pool capacity: %zu\n", pool_capacity(p));

}

#define Kibs(n) ((1<<10)*(n))

poolset_t* chain = NULL;

int main(int argc, char const *argv[])
{
	// unit_test1(argc, argv);
	// chain = poolset_init(malloc(Kibs(2048)), Kibs(2048), Kibs(4), 8);
	// void* t1 = poolset_pull(chain, 8);
	// assert(t1);
	// printf("biggest size: %zu\n", poolset_biggestsize(chain));

	gpallocator_t* main_thread = gpallocator_new
	((struct memman_hooks ){malloc, realloc, free}, Kibs(2048), 1, Kibs(4), 8, NULL);

	void* test = gpallocator_malloc(main_thread, 4064);
	assert(test);
	printf("[%zu]\n", payload_pool_segsize(test));

	return 0;
}
