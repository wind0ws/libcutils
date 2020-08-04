#pragma once
#ifndef __LCU_STRINGS_HEADER
#define __LCU_STRINGS_HEADER

#include <stdint.h>
#include <string.h>

#ifdef _WIN32
// strcasecmp is unix function.
#define strcasecmp stricmp
#define strncasecmp strnicmp
//to make MSC happy
#define stricmp _stricmp
#define strnicmp _strnicmp

/**
 * to make MSC happy
 * origin strcpy_s(dest, destSize, source)
 * here we assume that "dest" is big enough to storage "source", so we use 'strlen(source) + 1' as destSize.
 * be careful!
 */
#define strcpy(dest, source) strcpy_s(dest, strlen(source) + 1, source) 
#define strncpy(dest, source, max_count) strncpy_s(dest, (max_count) + 1, source, max_count)
#define strcat(dest, source) strcat_s(dest, strlen(dest) + strlen(source) + 1, source)
#define STRTOK_SAFE strtok_s
//#define snprintf(buf, buf_size, format, ...)  _snprintf_s(buf, buf_size, (buf_size) - 1, format, ## __VA_ARGS__)
#else
//in Unix platform. use strtok_r
#define STRTOK_SAFE strtok_r
//stricmp is windows function.
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif // _WIN32

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#if defined(__GLIBC__) || defined(_WIN32)
	/* Declaration of strlcpy() for platforms that don't already have it. */

	/**
     * Copy src to string dst of size size.  At most size-1 characters
     * will be copied.  Always NUL terminates (unless size == 0).
     * Returns strlen(src); if retval >= size, truncation occurred.
     */
	size_t strlcpy(char* dst, const char* src, size_t size);

	/**
     * Appends src to string dst of size "size" (unlike strncat, size is the
     * full size of dst, not space left).  At most size-1 characters
     * will be copied.  Always NUL terminates (unless size <= strlen(dst)).
     * Returns strlen(src) + MIN(size, strlen(initial dst)).
     * If retval >= size, truncation occurred.
     */
	size_t strlcat(char* dst, const char* src, size_t size);
#endif

	/**
	 * replace the "pattern" to "replacement" from "original".
	 * WARN: return string is malloced, need free after use!
	 * @param original: the str to replace
	 * @param pattern: the replace pattern
	 * @param replacement: replace pattern to this
	 */
	char* strreplace(char const* const original,
		char const* const pattern, char const* const replacement);

	/**
	 * split string by delimiter
	 * @param recv_splited_str: the pointer you want to receive spited string.
	 * @param p_splited_nums: how many char * do you provide in rec_splited_str,
	 *                        this value will rewrite to real split numbers after return.
	 * @param src_str: the origin str that you want to split
	 * @param delimiter: the string of delimiter
	 */
	void strsplit(char* recv_splited_str[], size_t* p_splited_nums,
		const char src_str[], const char* delimiter);

	/**
	 * count utf8 code points(words, NOT chars), NOT bytes.
	 * if you want to know number of bytes, use strlen/strnlen.
	 * uft8str must end with '\0', or memory will over read(that is dangerous).
	 * @return how many word in this string.
	 */
	size_t strutf8len(const char* utf8str);

	/**
	 * as same as strutf8len.
	 * but with max_count limitation: when reach '\0' or max_count, word search end.
	 * @return how many word in this string.
	 */
	size_t strnutf8len(const char* utf8str, size_t max_count);

#ifdef __cplusplus
};
#endif // __cplusplus

#endif // __LCU_STRINGS_HEADER