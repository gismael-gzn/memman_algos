#ifndef POOL_ALLCOATOR_H__
#define POOL_ALLCOATOR_H__

#include "align_config.h"
#include "arena.h"
#include <stdio.h>

typedef struct pool_t pool_t;

typedef struct idpool_t idpool_t;

pool_t* to_pool_t(idpool_t* ref);

idpool_t* to_idpool_t(pool_t* ref);

size_t cell_overhead_sizeof(void);

size_t idpool_sizeof(void);

size_t pool_sizeof(void);

size_t pool_segsize(pool_t* pool);

size_t pool_capacity(pool_t* pool);

pool_t* pool_init(void* mem, size_t mem_size, size_t segment);

pool_t* pool_new(malloc_impl* mallochook, size_t mem_size, size_t segment);

idpool_t* idpool_init(void* mem, size_t mem_size, size_t segment, void* id);

void* idpool_getid(idpool_t* pool);

void* pool_pull(pool_t* pool);

void* pool_pop_check(pool_t* pool, size_t n);

void pool_push(pool_t* pool, void* payload);

size_t payload_pool_segsize(void* payload);

pool_t* payload_owner_pool(void* payload);

void payload_pushto_pool(void* payload);

#endif //POOL_ALLCOATOR_H__
