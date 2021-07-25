#ifndef ALIGN_CONFIG_H__
#define ALIGN_CONFIG_H__

/* This determines the size and alignment of a granule, the smallest unit of
allocation handled by pools and the rest of data structures that use pools.
A granule is of size (1<<n) where n is the expansion of this macro.
A pool allocation refered to as 'cell' consists of at least 2 granules:
1. Stores info for deallocation, must not be overwritten
2. The actual memory payload, can be written in. 
That is, assuming that pointers are 8-bytes in size, under that assumption, 
this macro expects values >= 3, otherwise an 8-byte granule is assumed. */
#define GRANULE_SIZE_LOG2 (3)

/* This determines how the alignment requirements are implemented.
If compiled with -stdc=c99, the cells are padded with unions and VLAS.
If -stc=c11, the definition of a cell uses the C11 _Alignas keyword */
#if (__STDC__ == 1 && __STDC_VERSION__ >= 201112L)
#	define MEM_ALGOS_SHOULD_SUPPORT_STDALIGN
#endif

#endif //ALIGN_CONFIG_H__
