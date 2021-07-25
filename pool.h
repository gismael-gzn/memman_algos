#ifndef POOL_ALLCOATOR_H__
#define POOL_ALLCOATOR_H__

#include "align_config.h"
#include "arena.h"

typedef struct pool_t pool_t;

// inline these 2
size_t pool_segsize(pool_t* pool);

size_t pool_capacity(pool_t* pool);

pool_t* pool_init(void* mem, size_t mem_size, size_t segment);

pool_t* pool_new(malloc_impl* mallochook, size_t mem_size, size_t segment);

void* pool_pop(pool_t* pool);

void* pool_pop_check(pool_t* pool, size_t n);

void pool_push(pool_t* pool, void* payload);

size_t payload_size(void* payload);

void* payload_owner_pool(void* payload);

void payload_pushto_owner(void* payload);

#endif //POOL_ALLCOATOR_H__
