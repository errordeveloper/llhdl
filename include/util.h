#ifndef __UTIL_H
#define __UTIL_H

#include <string.h>
#include <stdlib.h>

/* Thanks Werner Almesberger for those */

#define alloc_size(s)					\
    ({  void *alloc_size_tmp = malloc(s);		\
	if(!alloc_size_tmp)				\
		abort();				\
	alloc_size_tmp; })

#define alloc_size0(s)					\
    ({  void *alloc_size_tmp2 = alloc_size(s);		\
	memset(alloc_size_tmp2, 0, s);			\
	alloc_size_tmp2; })

#define alloc_type(t) ((t *)alloc_size(sizeof(t)))
#define alloc_type0(t) ((t *)alloc_size0(sizeof(t)))

#define stralloc(s)					\
    ({  char *stralloc_tmp = strdup(s);			\
	if(!stralloc_tmp)				\
		abort();				\
	stralloc_tmp; })


static inline int min(int x, int y)
{
	return x < y ? x : y;
}

static inline int max(int x, int y)
{
	return x < y ? y : x;
}

#endif /* __UTIL_H */

