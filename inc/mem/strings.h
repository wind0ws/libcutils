#pragma once
#ifndef LCU_STRINGS_H
#define LCU_STRINGS_H

#include <stddef.h>
#include <string.h>
#include <ctype.h>
#ifndef _WIN32
#include <strings.h> //for bcopy/bzero
#endif // !_WIN32

#ifndef bcopy
#define bcopy(src, dest, len) memcpy((dest), (src), (len))
#endif // !bcopy
#ifndef bzero
#define bzero(b, len) memset((b), '\0', (len))
#endif // !bzero

#ifdef _WIN32
// strcasecmp is unix function. suggest to use strcasecmp for cross platform
#define strcasecmp(s1, s2)                   stricmp(s1, s2)
#define strncasecmp(s1, s2, n)               strnicmp(s1, s2, n)
//to make MSC happy
#define stricmp(s1, s2)                      _stricmp(s1, s2)
#define strnicmp(s1, s2, n)                  _strnicmp(s1, s2, n)
#ifndef strdup
#define strdup(s)                            _strdup(s)
#endif // !strdup
#ifndef strndup
	//windows not implement strndup, let's do it.
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
	char* strndup(const char* s, size_t n);
#ifdef __cplusplus
}
#endif // __cplusplus
#endif // !strndup

/**
 * to make MSC happy
 * origin strcpy_s(dest, destSize, src)
 * here we assume that "dest" is big enough to storage "src", so we use 'strlen(src) + 1' as destSize.
 * be careful!
 */
#define strcpy(dest, src)                    strcpy_s(dest, strlen(src) + 1, src) 
#define strncpy(dest, src, max_count)        strncpy_s(dest, (max_count) + 1, src, max_count)
#define strcat(dest, src)                    strcat_s(dest, strlen(dest) + strlen(src) + 1, src)
#define strtok_r(str, delimiter, ctx)        strtok_s(str, delimiter, ctx)
 //#define snprintf(buf, buf_size, format, ...)  _snprintf_s(buf, buf_size, (buf_size) - 1, format, ## __VA_ARGS__)
#else
//stricmp is windows function. suggest to use strcasecmp for cross platform
#define stricmp(s1, s2)                      strcasecmp(s1, s2)
#define strnicmp(s1, s2, n)                  strncasecmp(s1, s2, n)
#endif // _WIN32

#define _STRING_TRANSFORM(s, trans_func)     do{ for ( ; *p; ++p) *p = trans_func(*p); }while(0)
#define STRING2UPPER(s)                      _STRING_TRANSFORM(s, tolower)
#define STRING2LOWER(s)                      _STRING_TRANSFORM(s, toupper)

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

#endif // __GLIBC__ || _WIN32

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
		const char* src_str, const char* delimiter);

	/**
	 * trim string.
	 * Remove the part of the string from left and from right composed just of
	 * contiguous characters found in 'cset', that is a null terminted C string.
	 *
	 * Example:
	 *
	 * char s[64] = {0};
	 * strcpy(s, "AA...AA.a.aa.aHelloWorld     :::");
	 * strtrim(s,"Aa. :");
	 * printf("%s\n", s);
	 *
	 * Output will be just "HelloWorld".
	 *
	 * @param s: the string want to trim. this string must editable!!!
	 * @param cset: the char set want to be removed from s.
	 */
	void strtrim(char* s, const char* cset);

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

#endif // LCU_STRINGS_H
