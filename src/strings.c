#include "strings.h"
#include <malloc.h>

#if defined(__GLIBC__) || defined(_WIN32)

/* Implementation of strlcpy() for platforms that don't already have it. */

/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t
strlcpy(char* dst, const char* src, size_t siz)
{
	char* d = dst;
	const char* s = src;
	size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0) {
		while (--n != 0) {
			if ((*d++ = *s++) == '\0')
				break;
		}
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';		/* NUL-terminate dst */
		while (*s++)
			;
	}

	return(s - src - 1);	/* count does not include NUL */
}

#endif


//https://github.com/ssllab/temper1/blob/722991add4a6a239271e1f029ebe4daaad719496/strreplace.c
//warning: need free the return char pointer after use!
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
	for (oriptr = original; (patloc = strstr(oriptr, pattern)); oriptr = patloc + patlen)
	{
		patcnt++;
	}

	{
		// allocate memory for the new string
		size_t const retlen = orilen + patcnt * (replen - patlen);
		char* const returned = (char*)malloc(sizeof(char) * (retlen + 1));

		if (returned != NULL)
		{
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
