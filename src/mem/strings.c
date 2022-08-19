#include "mem/strings.h"
#include <malloc.h>

#ifdef _WIN32
char* strndup(const char* s, size_t n)
{
	if (!s)
	{
		return NULL;
	}
	// n == 0 is acceptable: just empty string.
	const size_t target_len = strnlen(s, n);
	char *target_str = (char *)malloc((target_len + 1U));
	if (!target_str)
	{
		return NULL;
	}
	if (target_len)
	{
		//for now(2020.10.15), the VS2019 16.7.6 report C6386 warning:
		//VS said that target_str maybe 0 byte array, i think that not possible!
		//so I'm sure there is no buffer overrun problem, disable warning.
#pragma warning(push)
#pragma warning(disable: 6386)
		memcpy(target_str, s, target_len);
#pragma warning(pop)
	}
	target_str[target_len] = '\0';
	return target_str;
}
#endif // _WIN32

#if defined(__GLIBC__) || defined(_WIN32)
/* Implementation of strlcpy() for platforms that don't already have it. */

size_t strlcpy(char* dst, const char* src, size_t size)
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

#endif // __GLIBC__ || _WIN32

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
	for (oriptr = original; NULL != (patloc = strstr(oriptr, pattern)); oriptr = patloc + patlen)
	{
		++patcnt;
	}

	// allocate memory for the new string
	size_t const retlen = orilen + patcnt * (replen - patlen);
	char* const returned = (char*)malloc(sizeof(char) * (retlen + 1));
	if (NULL != returned)
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
			//strncpy(retptr, oriptr, skplen);
			memcpy(retptr, oriptr, skplen);
			retptr += skplen;
			// copy the replacement 
			//strncpy(retptr, replacement, replen);
			memcpy(retptr, replacement, replen);
			retptr += replen;
		}
		// copy the rest of the string.
		strcpy(retptr, oriptr);
	}
	return returned;
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

void strtrim(char *s, const char *cset)
{
	char* start, * end, * sp, * ep;
	size_t len;

	sp = start = s;
	ep = end = s + strlen(s) - 1;
	while (sp <= end && strchr(cset, *sp)) sp++;
	while (ep > sp && strchr(cset, *ep)) ep--;
	len = (sp > ep) ? 0 : ((ep - sp) + 1);
	if (len && s != sp) memmove(s, sp, len);
	s[len] = '\0';
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

size_t str_char2hex(char* out_hex_str, size_t out_hex_str_capacity,
	const char* chars, size_t chars_count)
{
#define ONE_HEX_STR_SIZE (3U)
	size_t len_hex_str = 0U;
	out_hex_str[0] = '\0';

	if (chars_count * ONE_HEX_STR_SIZE >= out_hex_str_capacity)
	{
		len_hex_str = snprintf(out_hex_str, out_hex_str_capacity - 1U, "hex truncated(%zu):", chars_count);
		chars_count = out_hex_str_capacity / ONE_HEX_STR_SIZE - 1U;
	}
	for (size_t chars_index = 0U; chars_index < chars_count; ++chars_index)
	{
		int ret_sn = snprintf(out_hex_str + len_hex_str,
			out_hex_str_capacity - len_hex_str - 1,
			" %02x", (unsigned char)chars[chars_index]);
		if (ONE_HEX_STR_SIZE != ret_sn /*ret_sn < 0*/) /* should be ONE_HEX_STR_SIZE */
		{
			break; // oops, error occurred
		}
		len_hex_str += ret_sn;
	}
	return len_hex_str;
}
