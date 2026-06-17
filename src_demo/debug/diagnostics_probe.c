#include "mem/mem_debug.h"
#include "common_macro.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _WIN32
static int spawn_probe(const char *mode, PROCESS_INFORMATION *process_info)
{
	char executable[MAX_PATH];
	char command_line[MAX_PATH * 2];
	STARTUPINFOA startup_info;

	if (GetModuleFileNameA(NULL, executable, MAX_PATH) == 0)
	{
		return -1;
	}
	snprintf(command_line, sizeof(command_line), "\"%s\" %s", executable, mode);
	memset(&startup_info, 0, sizeof(startup_info));
	startup_info.cb = sizeof(startup_info);
	memset(process_info, 0, sizeof(*process_info));
	if (!CreateProcessA(NULL, command_line, NULL, NULL, TRUE, 0, NULL, NULL,
		&startup_info, process_info))
	{
		return -1;
	}
	return 0;
}

static int wait_for_file(const char *path, DWORD timeout_ms)
{
	DWORD start = GetTickCount();
	while (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES)
	{
		if (GetTickCount() - start >= timeout_ms)
		{
			return -1;
		}
		Sleep(10);
	}
	return 0;
}

static int count_diagnostics_logs(void)
{
	WIN32_FIND_DATAA find_data;
	HANDLE find_handle = FindFirstFileA("lcu_diagnostics*.log", &find_data);
	int count = 0;
	if (find_handle == INVALID_HANDLE_VALUE)
	{
		return 0;
	}
	do
	{
		if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			++count;
		}
	} while (FindNextFileA(find_handle, &find_data));
	FindClose(find_handle);
	return count;
}

static int run_concurrency_probe(void)
{
	PROCESS_INFORMATION holder;
	PROCESS_INFORMATION writer;
	DWORD exit_code;

	DeleteFileA("holder.ready");
	if (spawn_probe("hold_primary", &holder) != 0 ||
		wait_for_file("holder.ready", 3000) != 0)
	{
		return 70;
	}
	if (spawn_probe("write_secondary", &writer) != 0)
	{
		TerminateProcess(holder.hProcess, 71);
		return 71;
	}
	WaitForSingleObject(writer.hProcess, 5000);
	GetExitCodeProcess(writer.hProcess, &exit_code);
	CloseHandle(writer.hThread);
	CloseHandle(writer.hProcess);
	WaitForSingleObject(holder.hProcess, 5000);
	CloseHandle(holder.hThread);
	CloseHandle(holder.hProcess);
	DeleteFileA("holder.ready");

	if (exit_code != 0 || count_diagnostics_logs() != 1 ||
		GetFileAttributesA("lcu_diagnostics.log") != INVALID_FILE_ATTRIBUTES)
	{
		return 72;
	}
	printf("concurrency probe passed\n");
	return 0;
}

static int run_retention_probe(void)
{
	char path[64];
	FILE *file;
	int i;

	for (i = 1000; i < 1040; ++i)
	{
		snprintf(path, sizeof(path), "lcu_diagnostics.%d.log", i);
		file = fopen(path, "wb");
		if (!file)
		{
			return 73;
		}
		fputs("old diagnostics\n", file);
		fclose(file);
	}
	MEM_CHECK_INIT();
	if (count_diagnostics_logs() != 32)
	{
		MEM_CHECK_DEINIT();
		return 74;
	}
	MEM_CHECK_DEINIT();
	printf("retention probe passed\n");
	return 0;
}
#endif

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		fprintf(stderr, "usage: diagnostics_probe <mode>\n");
		return 64;
	}

	if (0 == strcmp(argv[1], "assert_config"))
	{
		MEM_CHECK_INIT();
#if defined(_DEBUG)
		ASSERT(0);
#else
		ASSERT_ABORT(0);
#endif
		return 65;
	}

#if defined(_USE_LCU_MEM_CHECK)
	if (0 == strcmp(argv[1], "tracker_leak"))
	{
		void *leaked;
		MEM_CHECK_INIT();
		leaked = malloc(64);
		if (!leaked)
		{
			return 66;
		}
		memset(leaked, 0x5A, 64);
		MEM_CHECK_DEINIT();
		return 0;
	}
	if (0 == strcmp(argv[1], "tracker_corruption"))
	{
		unsigned char *allocation;
		MEM_CHECK_INIT();
		allocation = (unsigned char *)malloc(8);
		if (!allocation)
		{
			return 66;
		}
		allocation[8] ^= 0xFF;
		free(allocation);
		return 67;
	}
#endif

#if defined(_WIN32) && defined(_DEBUG)
	if (0 == strcmp(argv[1], "crt_warn"))
	{
		MEM_CHECK_INIT();
		_RPT0(_CRT_WARN, "diagnostics probe CRT warning\n");
		MEM_CHECK_DEINIT();
		return 0;
	}
	if (0 == strcmp(argv[1], "crt_leak"))
	{
		void *leaked;
		MEM_CHECK_INIT();
		leaked = malloc(64);
		if (!leaked)
		{
			return 66;
		}
		memset(leaked, 0xA5, 64);
		MEM_CHECK_DEINIT();
		return 0;
	}
	if (0 == strcmp(argv[1], "primary_write"))
	{
		MEM_CHECK_INIT();
		lcu_diagnostics_write("PROBE", "primary diagnostic");
		MEM_CHECK_DEINIT();
		return 0;
	}
	if (0 == strcmp(argv[1], "hold_primary"))
	{
		FILE *ready;
		MEM_CHECK_INIT();
		ready = fopen("holder.ready", "wb");
		if (!ready)
		{
			return 67;
		}
		fclose(ready);
		Sleep(2000);
		MEM_CHECK_DEINIT();
		return 0;
	}
	if (0 == strcmp(argv[1], "write_secondary"))
	{
		MEM_CHECK_INIT();
		lcu_diagnostics_write("PROBE", "secondary diagnostic");
		MEM_CHECK_DEINIT();
		return 0;
	}
	if (0 == strcmp(argv[1], "concurrency"))
	{
		return run_concurrency_probe();
	}
	if (0 == strcmp(argv[1], "retention"))
	{
		return run_retention_probe();
	}
#endif

	fprintf(stderr, "unknown diagnostics probe mode: %s\n", argv[1]);
	return 64;
}
