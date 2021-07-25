#include "poolset.h"
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
		mem[i] = pool_pop(p);
		memset(mem[i], 7777, payload_size(mem[i]));
		assert(payload_owner_pool(mem[i]) == p);
		assert((uintptr_t)mem[i]%8==0);
	}
	printf("pool capacity: %zu\n", pool_capacity(p));

	for(size_t i=0; i<m; ++i)
		payload_pushto_owner(mem[i]);
	printf("pool capacity: %zu\n", pool_capacity(p));

}

#define Kibs(n) ((1<<10)*(n))

_Thread_local poolset_t* chain = NULL;

int main(int argc, char const *argv[])
{
	// unit_test1(argc, argv);
	printf("poolchain size: %u\n", Kibs(256));

	chain = poolset_init(malloc(Kibs(256)), Kibs(256), 4096, 8);

	return 0;
}
