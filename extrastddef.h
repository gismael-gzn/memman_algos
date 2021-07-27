#ifndef EXTRA_STDDEFF_H__
#define EXTRA_STDDEFF_H__

#include <stddef.h>
#include <stdint.h>

typedef uint8_t byte_t;

#define isnull(ptr) (ptr == NULL)

#define byteptr(ptr) ((byte_t*)ptr)

#define ptroff(begin, end) (byteptr(end)-byteptr(begin))

#define to_nearest_multiple(x, mul) ((x==0?1:x)%(mul)==0? 0: ((mul)-((x)%(mul))))

#endif //EXTRA_STDDEFF_H__
