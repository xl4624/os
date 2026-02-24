#pragma once

#include <sys/cdefs.h>

#ifndef _SIZE_T
#define _SIZE_T
typedef __SIZE_TYPE__ size_t;
#endif

#ifndef _PTRDIFF_T
#define _PTRDIFF_T
typedef __PTRDIFF_TYPE__ ptrdiff_t;
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

#define offsetof(T, member) __builtin_offsetof(T, member)
