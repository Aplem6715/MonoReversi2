
#if !defined(_DEBUG_UTIL_H_)
#define _DEBUG_UTIL_H_

#ifdef DEBUG
#include <stdio.h>
#define DEBUG_PUTS(str) puts(str)
#define DEBUG_PRINTF(fmt, ...) printf(fmt, __VA_ARGS__);
#else
#define DEBUG_PUTS(str)
#define DEBUG_PRINTF(fmt, ...)
#endif

#endif // _DEBUG_UTIL_H_
