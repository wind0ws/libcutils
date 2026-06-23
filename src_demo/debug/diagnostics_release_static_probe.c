#include "mem/mem_debug.h"

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
