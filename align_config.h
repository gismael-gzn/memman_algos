#ifndef ALIGN_CONFIG_H__
#define ALIGN_CONFIG_H__

/* A granule is considered to be the smallest unit of allocation which size
is a power of 2, that is (1<<GRANULE_SIZE_LOG2).
The size of a granule also determines its alignment, so, as all allocations
consist of a set of contiguous granules, allocators such as memory pools
or the gpallocator, by a series of compile-time heuristics?, 
try to align their data structure definitions so that their payload (allocation) 
has the alignment of a granule.
If this macro has a value <3, then it gets redefined to be == 3 in cell.h */
#define GRANULE_SIZE_LOG2 (3)

#endif //ALIGN_CONFIG_H__
