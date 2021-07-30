#include "arena.h"

struct arena_t
{
	byte_t *alloc_ptr, *end;
};

size_t arena_sizeof(void)
{
	return sizeof(arena_t);
}

size_t bytes_to_align(void* ptr, size_t align)
{
	return to_nearest_multiple((uint64_t)(byteptr(ptr)+sizeof(arena_t)), align);
}

void* align_pointer(void* ptr, size_t alignment)
{
	return (byteptr(ptr) + bytes_to_align(ptr, alignment));
}

static inline void* linear_malloc(byte_t** mem, size_t n)
{
	void* payload = *mem;
	*mem += n;
	return payload;
}

size_t arena_capacity(arena_t* reg)
{
	return ptroff(reg->alloc_ptr, reg->end);
}

size_t arena_size(arena_t* reg)
{
	return ptroff(reg, reg->end);
}

arena_t* arena_init(void* mem, size_t mem_size, size_t alignment)
{
	arena_t* reg = mem;

	size_t min_required_size = sizeof *reg;
	min_required_size += to_nearest_multiple(sizeof(reg), alignment);
	if(mem_size <= min_required_size)
		return NULL;

	if (!isnull(reg))
		*reg = (arena_t){
			byteptr(mem) + sizeof *reg + bytes_to_align(mem, alignment),
			byteptr(mem) + mem_size,
		};

	return reg;
}

arena_t* arena_new(malloc_impl* mallochook, size_t mem_size, size_t alignment)
{
	if(isnull(mallochook)) mallochook = malloc;
	void* mem = mallochook(mem_size);
	return arena_init(mem, mem_size, alignment);
}

void* arena_malloc(arena_t* reg, size_t n)
{
	return n<=arena_capacity(reg)? linear_malloc(&reg->alloc_ptr, n): NULL;
}

void* arena_malloc_quick(arena_t* reg, size_t n)
{
	return linear_malloc(&reg->alloc_ptr, n);
}
