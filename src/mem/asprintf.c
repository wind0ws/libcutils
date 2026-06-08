#include "mem/mem_debug.h"
#include "mem/asprintf.h"

/* ============================================================================
 * OWNERSHIP TRANSFER - DO NOT TRACK
 * ============================================================================
 * asprintf/vasprintf are STANDARD POSIX/GNU function names with a universal
 * contract: the returned buffer (*strp) is released by the caller with libc
 * free(). The entire world expects this behavior.
 *
 * We undo the mem_debug.h malloc/free rewrite here so this implementation uses
 * raw libc malloc/free. Using lcu_malloc_trace would return a canary-offset/
 * tracked pointer that corrupts the heap when the caller's free() runs while
 * the allocation tracker is active.
 *
 * DO NOT remove these #undef lines. DO NOT change malloc() to lcu_malloc_trace.
 *
 * Internal lcu callers (e.g. str_params.c) that include mem_debug.h must use
 * lcu_free_raw() to release asprintf results, NOT bare free() (which is
 * rewritten to lcu_free and would crash on this untracked pointer).
 *
 * External callers use standard libc free().
 * See: allocator.h (lcu_malloc_raw/lcu_free_raw), ownership_contract_test.c
 * ============================================================================ */
#ifdef malloc
#undef malloc
#endif
#ifdef free
#undef free
#endif

#ifdef __GNUC__
int vscprintf(const char* format, va_list ap)
{
	va_list ap_copy;
	va_copy(ap_copy, ap);
	int retval = vsnprintf(NULL, 0, format, ap_copy);
	va_end(ap_copy);
	return retval;
}
#endif // __GNUC__

#ifdef _MSC_VER
int vasprintf(char** strp, const char* format, va_list ap)
{
	int len = vscprintf(format, ap);
	if (len == -1)
	{
		return -1;
	}
	char* str = (char*)malloc((size_t)len + 1);
	if (!str)
	{
		return -1;
	}

	int retval = vsnprintf(str, len + 1, format, ap);
	if (retval == -1)
	{
		free(str);
		return -1;
	}
	*strp = str;
	return retval;
}

int asprintf(char** strp, const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	int retval = vasprintf(strp, format, ap);
	va_end(ap);
	return retval;
}
#endif // _MSC_VER

