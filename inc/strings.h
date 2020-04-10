#pragma once
#ifndef __STRINGS_HEADER__
#define __STRINGS_HEADER__

#include <stdint.h>
#include <sys/types.h>
#include <string.h>

#ifdef _WIN32
//to make MSC happy
#define stricmp _stricmp
/**
 * to make MSC happy
 * origin strcpy_s(dest, destSize, source)
 * here we assume that "dest" is big enough to storage "source", so we use 'strlen(source) + 1' as destSize.
 * be careful! 
 */
#define strcpy(dest, source) strcpy_s(dest, strlen(source) + 1, source) 
#define strncpy(dest, source, max_count) strncpy_s(dest, (max_count) + 1, source, max_count)
#define strcat(dest, source) strcat_s(dest, strlen(dest) + strlen(source) + 1, source)
#endif // _WIN32

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#if defined(__GLIBC__) || defined(_WIN32)
	/* Declaration of strlcpy() for platforms that don't already have it. */
	size_t strlcpy(char* dst, const char* src, size_t size);
#endif
	/**
	 * replace the "pattern" value to "replacement" from "original".
	 * warning: return value is malloced, need free after use!
	 * @param original 
	 */
	char* strreplace(char const* const original,
		char const* const pattern, char const* const replacement);

#ifdef __cplusplus
};
#endif // __cplusplus

#endif //__STRINGS_HEADER__