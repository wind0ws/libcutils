#pragma once
#ifndef __STRINGS_HEADER__
#define __STRINGS_HEADER__

#include <stdint.h>
#include <sys/types.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#if defined(__GLIBC__) || defined(_WIN32)
	/* Declaration of strlcpy() for platforms that don't already have it. */
	size_t strlcpy(char* dst, const char* src, size_t size);
#endif

#ifdef __cplusplus
};
#endif // __cplusplus

#endif //__STRINGS_HEADER__