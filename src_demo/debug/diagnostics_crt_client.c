#include "mem/mem_debug.h"

#include <crtdbg.h>

__declspec(dllexport) void lcu_diagnostics_crt_client_run(void)
{
	MEM_CHECK_INIT();
	_RPT0(_CRT_WARN, "client DLL CRT warning\n");
}

__declspec(dllexport) void lcu_diagnostics_crt_client_stop(void)
{
	MEM_CHECK_DEINIT();
}
