#ifndef QUICK_FUNCTIONS_H__
#define QUICK_FUNCTIONS_H__

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

typedef union number
{
	uint64_t usg;
	int64_t sg;
}number;

static inline number toint(const char* buf, unsigned base)
{
	size_t buflen = strlen(buf);

	char tmpbuf[8192] = {0}, *p=tmpbuf;
	memcpy(tmpbuf, buf, buflen);
	for(; *p != '\0' && *p != '-'; ++p);

	number retval = {0};

	if(*p == '-')
		retval.sg = strtoll(buf, &p, base);
	else
		retval.usg = strtoull(buf, &p, base);

	return retval;
}

#endif //QUICK_FUNCTIONS_H__


