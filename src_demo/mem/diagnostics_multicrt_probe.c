#include "mem/mem_debug.h"

#include <crtdbg.h>
#include <stdio.h>
#include <string.h>

__declspec(dllimport) void lcu_diagnostics_crt_client_run(void);
__declspec(dllimport) void lcu_diagnostics_crt_client_stop(void);

static int log_contains_both_reports(void)
{
	char content[8192];
	FILE *file = fopen("lcu_diagnostics.log", "rb");
	size_t size;

	if (!file)
	{
		return 0;
	}
	size = fread(content, 1, sizeof(content) - 1, file);
	fclose(file);
	content[size] = '\0';
	return strstr(content, "executable CRT warning") != NULL &&
		strstr(content, "client DLL CRT warning") != NULL;
}

int main(int argc, char **argv)
{
	(void)argv;
	if (argc != 2)
	{
		return 64;
	}

	MEM_CHECK_INIT();
	_RPT0(_CRT_WARN, "executable CRT warning\n");
	lcu_diagnostics_crt_client_run();
	lcu_diagnostics_crt_client_stop();
	MEM_CHECK_DEINIT();

	if (!log_contains_both_reports())
	{
		return 80;
	}
	printf("multi CRT probe passed\n");
	return 0;
}
