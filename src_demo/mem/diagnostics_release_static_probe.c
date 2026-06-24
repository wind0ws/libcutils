#include "mem/mem_debug.h"

/* This file intentionally models a client translation unit:
 * - the client includes mem_debug.h first,
 * - the client is built as Debug,
 * - the client links the Release lcu_static library.
 * The leaked malloc below must report this source file via _CRTDBG_MAP_ALLOC. */
#include <crtdbg.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
	void *leaked;

	MEM_CHECK_INIT();
	_RPT0(_CRT_WARN, "release static debug client CRT warning\n");
	leaked = malloc(64);
	if (!leaked)
	{
		MEM_CHECK_DEINIT();
		return 66;
	}
	memset(leaked, 0xC3, 64);
	MEM_CHECK_DEINIT();
	return 0;
}
