#include "mem/strings.h"
#include <malloc.h>

#if defined(__GLIBC__) || defined(_WIN32)

/* Implementation of strlcpy() for platforms that don't already have it. */

/*
 * Copy src to string dst of size size.  At most size-1 characters
 * will be copied.  Always NUL terminates (unless size == 0).
 * Returns strlen(src); if retval >= size, truncation occurred.
 */
size_t
strlcpy(char* dst, const char* src, size_t size)
{
	char* d = dst;
	const char* s = src;
	size_t n = size;

	/* Copy as many bytes as will fit */
	if (n != 0) 
	{
		while (--n != 0) 
		{
			if ((*d++ = *s++) == '\0') 
			{
				break;
			}
		}
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) 
	{
		if (size != 0)
		{
			*d = '\0';		/* NUL-terminate dst */
		}
		while (*s++)
			;
	}

	return (s - src - 1);	/* count does not include NUL */
}

/*
 * Appends src to string dst of size "size" (unlike strncat, size is the
 * full size of dst, not space left).  At most size-1 characters
 * will be copied.  Always NUL terminates (unless size <= strlen(dst)).
 * Returns strlen(src) + MIN(size, strlen(initial dst)).
 * If retval >= size, truncation occurred.
 */
size_t strlcat(char* dst, const char* src, size_t size)
{
	char* d = dst;
	const char* s = src;
	size_t n = size;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0 && *d != '\0') 
	{
		++d;
	}
	dlen = d - dst;
	n = size - dlen;

	if (n == 0) 
	{
		return (dlen + strlen(s));
	}

	while (*s != '\0') 
	{
		if (n != 1) 
		{
			*d++ = *s;
			--n;
		}
		++s;
	}
	*d = '\0';

	return (dlen + (s - src));       /* count does not include NUL */
}

#endif

//https://github.com/ssllab/temper1/blob/722991add4a6a239271e1f029ebe4daaad719496/strreplace.c
//warning: need free the return pointer after use!
char* strreplace(char const* const original,
	char const* const pattern, char const* const replacement)
{
	size_t const replen = strlen(replacement);
	size_t const patlen = strlen(pattern);
	size_t const orilen = strlen(original);

	size_t patcnt = 0;
	const char* oriptr;
	const char* patloc;

	// find how many times the pattern occurs in the original string
	for (oriptr = original; (patloc = strstr(oriptr, pattern)) != NULL; oriptr = patloc + patlen)
	{
		++patcnt;
	}

	{
		// allocate memory for the new string
		size_t const retlen = orilen + patcnt * (replen - patlen);
		char* const returned = (char*)malloc(sizeof(char) * (retlen + 1));
		if (returned != NULL)
		{
			//memset(returned, '\0', sizeof(char) * (retlen + 1));
			returned[0] = '\0';
			// copy the original string, 
			// replacing all the instances of the pattern
			char* retptr = returned;
			for (oriptr = original; (patloc = strstr(oriptr, pattern)); oriptr = patloc + patlen)
			{
				size_t const skplen = patloc - oriptr;
				// copy the section until the occurrence of the pattern
				strncpy(retptr, oriptr, skplen);
				retptr += skplen;
				// copy the replacement 
				strncpy(retptr, replacement, replen);
				retptr += replen;
			}
			// copy the rest of the string.
			strcpy(retptr, oriptr);
		}
		return returned;
	}
}

void strsplit(char* recv_splited_str[], size_t* p_splited_nums, const char *src_str, const char* delimiter)
{
	char* token = NULL;
	char* token_ctx = NULL;
	size_t recv_ptrs_size = *p_splited_nums;
	*p_splited_nums = 0;

	token = strtok_r((char *)src_str, delimiter, &token_ctx);
	while (token && *p_splited_nums < recv_ptrs_size)
	{
		recv_splited_str[*p_splited_nums] = token;
		(*p_splited_nums)++;

		token = strtok_r(NULL, delimiter, &token_ctx);
	}
}

size_t strutf8len(const char* utf8str)
{
	return strnutf8len(utf8str, strlen(utf8str));
}

size_t strnutf8len(const char* utf8str, size_t max_count)
{
	size_t code_points = 0;
	size_t chars_count = 0;
	while (*utf8str && chars_count < max_count) 
	{
		code_points += ((*utf8str & 0xC0) != 0x80);
		++utf8str;
		++chars_count;
	}
	return code_points;
}
