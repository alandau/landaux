#ifndef ERR_H
#define ERR_H

#define MAX_ERRNO 4095

static inline void *ERR_PTR(long val)
{
	return (void *)val;
}

static inline long PTR_ERR(const void *ptr)
{
	return (long)ptr;
}

static inline int IS_ERR(const void *ptr)
{
	return (unsigned long)ptr >= (unsigned long)-MAX_ERRNO;
}

#endif
