#include "gpallocator.h"
#include "quicks.h.c"
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
	// chain = poolset_new(malloc, 8, 504, NULL);
	// void* t1 = poolset_pull(chain, toint(argv[1], 10).usg);
	// assert(t1);
	// printf("biggest size: %zu\n", poolset_biggestsize(chain));
	// size_t sz = gpallocated_size(t1);
	// printf("size is %zu\n", sz);

	gpallocator_t* main_thread = gpallocator_new
	((struct memman_hooks ){0}, 1, 8, 512, main);

	void *test = gpallocator_malloc(main_thread, toint(argv[1], 10).sg);
	size_t sz = gpallocated_size(test);
	printf("size is %zu\n", sz);

	// while(1)
	// {
	// 	void* intense_test =
	// 	gpallocator_malloc(main_thread, toint(argv[1], 10).sg);
	// 	assert(intense_test);
	// // printf("[%zu]\n", payload_pool_segsize(intense_test));
	// }

	// void* test = gpallocator_malloc(main_thread, toint(argv[1], 10).sg);
	// assert(test);
	// printf("[%zu]\n", payload_pool_segsize(test));

	return 0;
}
