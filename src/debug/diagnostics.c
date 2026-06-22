#include "debug/diagnostics.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32

#pragma warning(push)
#pragma warning(disable : 5105)
#include <windows.h>
#include <strsafe.h>
#pragma warning(pop)

#define LCU_DIAGNOSTICS_MAX_CRTS 16
#define LCU_DIAGNOSTICS_MAX_MESSAGE 4096
#define LCU_DIAGNOSTICS_MAX_OVERFLOW_LOGS 32

typedef struct lcu_diagnostics_file_entry
{
	wchar_t path[MAX_PATH];
	FILETIME last_write;
} lcu_diagnostics_file_entry_t;

#ifdef _DEBUG
typedef struct lcu_diagnostics_crt_entry
{
	lcu_diagnostics_crt_api_t api;
	unsigned int references;
} lcu_diagnostics_crt_entry_t;

static SRWLOCK g_crt_lock = SRWLOCK_INIT;
static lcu_diagnostics_crt_entry_t g_crt_entries[LCU_DIAGNOSTICS_MAX_CRTS];
static size_t g_crt_count = 0;
#endif

static INIT_ONCE g_sink_once = INIT_ONCE_STATIC_INIT;
static wchar_t g_log_path[MAX_PATH];
static wchar_t g_started_env[96];
static HANDLE g_process_write_mutex = NULL;
static HANDLE g_primary_semaphore = NULL;

#ifdef _DEBUG
static int __cdecl lcu_diagnostics_crt_hook(int report_type, char *message, int *return_value);
static int __cdecl lcu_diagnostics_crt_hook_wide(int report_type, wchar_t *message, int *return_value);
#endif

static unsigned long long lcu_diagnostics_hash_wide(const wchar_t *text)
{
	unsigned long long hash = 1469598103934665603ULL;
	while (*text)
	{
		hash ^= (unsigned short)*text++;
		hash *= 1099511628211ULL;
	}
	return hash;
}

static int lcu_diagnostics_file_entry_compare(const void *left, const void *right)
{
	const lcu_diagnostics_file_entry_t *lhs = (const lcu_diagnostics_file_entry_t *)left;
	const lcu_diagnostics_file_entry_t *rhs = (const lcu_diagnostics_file_entry_t *)right;
	LONG high_compare = CompareFileTime(&lhs->last_write, &rhs->last_write);
	if (high_compare != 0)
	{
		return high_compare;
	}
	return _wcsicmp(lhs->path, rhs->path);
}

static int lcu_diagnostics_is_pid_log_name(const wchar_t *name)
{
	const wchar_t prefix[] = L"lcu_diagnostics.";
	const wchar_t suffix[] = L".log";
	size_t name_len = wcslen(name);
	size_t prefix_len = (sizeof(prefix) / sizeof(prefix[0])) - 1;
	size_t suffix_len = (sizeof(suffix) / sizeof(suffix[0])) - 1;
	size_t i;

	if (name_len <= prefix_len + suffix_len ||
		0 != wcsncmp(name, prefix, prefix_len) ||
		0 != wcscmp(name + name_len - suffix_len, suffix))
	{
		return 0;
	}
	for (i = prefix_len; i < name_len - suffix_len; ++i)
	{
		if (name[i] < L'0' || name[i] > L'9')
		{
			return 0;
		}
	}
	return 1;
}

static void lcu_diagnostics_cleanup_overflow_logs(const wchar_t *directory, size_t keep_count)
{
	wchar_t pattern[MAX_PATH];
	WIN32_FIND_DATAW find_data;
	HANDLE find_handle;
	lcu_diagnostics_file_entry_t *entries = NULL;
	size_t count = 0;
	size_t capacity = 0;

	if (FAILED(StringCchPrintfW(pattern, MAX_PATH, L"%s\\lcu_diagnostics.*.log", directory)))
	{
		return;
	}

	find_handle = FindFirstFileW(pattern, &find_data);
	if (find_handle == INVALID_HANDLE_VALUE)
	{
		return;
	}

	do
	{
		lcu_diagnostics_file_entry_t *new_entries;
		size_t new_capacity;

		if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
			!lcu_diagnostics_is_pid_log_name(find_data.cFileName))
		{
			continue;
		}
		if (count == capacity)
		{
			new_capacity = capacity == 0 ? 32 : capacity * 2;
			if (entries)
			{
				new_entries = (lcu_diagnostics_file_entry_t *)HeapReAlloc(
					GetProcessHeap(), HEAP_ZERO_MEMORY, entries,
					new_capacity * sizeof(lcu_diagnostics_file_entry_t));
			}
			else
			{
				new_entries = (lcu_diagnostics_file_entry_t *)HeapAlloc(
					GetProcessHeap(), HEAP_ZERO_MEMORY,
					new_capacity * sizeof(lcu_diagnostics_file_entry_t));
			}
			if (!new_entries)
			{
				break;
			}
			entries = new_entries;
			capacity = new_capacity;
		}

		if (FAILED(StringCchPrintfW(entries[count].path, MAX_PATH,
			L"%s\\%s", directory, find_data.cFileName)))
		{
			continue;
		}
		entries[count].last_write = find_data.ftLastWriteTime;
		++count;
	} while (FindNextFileW(find_handle, &find_data));

	FindClose(find_handle);

	if (count > keep_count)
	{
		size_t delete_count = count - keep_count;
		size_t i;
		qsort(entries, count, sizeof(entries[0]), lcu_diagnostics_file_entry_compare);
		for (i = 0; i < delete_count; ++i)
		{
			DeleteFileW(entries[i].path);
		}
	}
	if (entries)
	{
		HeapFree(GetProcessHeap(), 0, entries);
	}
}

static BOOL CALLBACK lcu_diagnostics_sink_init_once(PINIT_ONCE init_once, PVOID parameter, PVOID *context)
{
	wchar_t directory[MAX_PATH];
	wchar_t path_env[96];
	wchar_t cleanup_env[96];
	wchar_t init_mutex_name[96];
	wchar_t semaphore_name[128];
	wchar_t write_mutex_name[128];
	wchar_t marker[2];
	DWORD process_id = GetCurrentProcessId();
	HANDLE init_mutex;
	unsigned long long directory_hash;
	DWORD directory_len;
	DWORD path_len;
	int primary = 0;

	(void)init_once;
	(void)parameter;
	(void)context;

	directory_len = GetCurrentDirectoryW(MAX_PATH, directory);
	if (directory_len == 0 || directory_len >= MAX_PATH)
	{
		wcscpy_s(directory, MAX_PATH, L".");
	}
	directory_hash = lcu_diagnostics_hash_wide(directory);

	StringCchPrintfW(path_env, 96, L"_LCU_DIAGNOSTICS_PATH_%lu", process_id);
	StringCchPrintfW(cleanup_env, 96, L"_LCU_DIAGNOSTICS_CLEANED_%lu", process_id);
	StringCchPrintfW(g_started_env, 96, L"_LCU_DIAGNOSTICS_STARTED_%lu_%016llx",
		process_id, directory_hash);
	StringCchPrintfW(init_mutex_name, 96, L"Local\\LCU.Diagnostics.Init.%lu", process_id);
	StringCchPrintfW(write_mutex_name, 128, L"Local\\LCU.Diagnostics.Write.%lu.%016llx",
		process_id, directory_hash);

	init_mutex = CreateMutexW(NULL, FALSE, init_mutex_name);
	if (init_mutex)
	{
		WaitForSingleObject(init_mutex, INFINITE);
	}

	path_len = GetEnvironmentVariableW(path_env, g_log_path, MAX_PATH);
	if (path_len == 0 || path_len >= MAX_PATH)
	{
		StringCchPrintfW(semaphore_name, 128,
			L"Local\\LCU.Diagnostics.Primary.%016llx", directory_hash);
		g_primary_semaphore = CreateSemaphoreW(NULL, 1, 1, semaphore_name);
		if (g_primary_semaphore &&
			WaitForSingleObject(g_primary_semaphore, 0) == WAIT_OBJECT_0)
		{
			primary = 1;
			StringCchPrintfW(g_log_path, MAX_PATH,
				L"%s\\lcu_diagnostics.log", directory);
		}
		else
		{
			StringCchPrintfW(g_log_path, MAX_PATH,
				L"%s\\lcu_diagnostics.%lu.log", directory, process_id);
		}
		SetEnvironmentVariableW(path_env, g_log_path);

		if (GetEnvironmentVariableW(cleanup_env, marker, 2) == 0)
		{
			lcu_diagnostics_cleanup_overflow_logs(directory,
				primary ? LCU_DIAGNOSTICS_MAX_OVERFLOW_LOGS :
				LCU_DIAGNOSTICS_MAX_OVERFLOW_LOGS - 1);
			SetEnvironmentVariableW(cleanup_env, L"1");
		}
	}

	g_process_write_mutex = CreateMutexW(NULL, FALSE, write_mutex_name);

	if (init_mutex)
	{
		ReleaseMutex(init_mutex);
		CloseHandle(init_mutex);
	}
	return TRUE;
}

static void lcu_diagnostics_ensure_sink(void)
{
	InitOnceExecuteOnce(&g_sink_once, lcu_diagnostics_sink_init_once, NULL, NULL);
}

static void lcu_diagnostics_write_stderr(const char *message, size_t message_len)
{
	HANDLE stderr_handle = GetStdHandle(STD_ERROR_HANDLE);
	DWORD written;

	if (stderr_handle != NULL && stderr_handle != INVALID_HANDLE_VALUE)
	{
		WriteFile(stderr_handle, message, (DWORD)message_len, &written, NULL);
	}
}

static void lcu_diagnostics_write_log(const char *message, size_t message_len)
{
	HANDLE log_file;
	DWORD written;
	wchar_t marker[2];
	int first_write;

	lcu_diagnostics_ensure_sink();
	if (g_log_path[0] == L'\0')
	{
		return;
	}

	if (g_process_write_mutex)
	{
		WaitForSingleObject(g_process_write_mutex, INFINITE);
	}

	first_write = GetEnvironmentVariableW(g_started_env, marker, 2) == 0;
	log_file = CreateFileW(g_log_path, FILE_APPEND_DATA,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, first_write ? CREATE_ALWAYS : OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (log_file != INVALID_HANDLE_VALUE)
	{
		if (first_write)
		{
			char header[256];
			SYSTEMTIME now;
			int header_len;
			GetLocalTime(&now);
			header_len = snprintf(header, sizeof(header),
				"=== libcutils diagnostics session %04u-%02u-%02u %02u:%02u:%02u.%03u pid=%lu ===\r\n",
				now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute,
				now.wSecond, now.wMilliseconds, GetCurrentProcessId());
			if (header_len > 0)
			{
				WriteFile(log_file, header, (DWORD)header_len, &written, NULL);
			}
			SetEnvironmentVariableW(g_started_env, L"1");
		}
		WriteFile(log_file, message, (DWORD)message_len, &written, NULL);
		FlushFileBuffers(log_file);
		CloseHandle(log_file);
	}

	if (g_process_write_mutex)
	{
		ReleaseMutex(g_process_write_mutex);
	}
}

static void lcu_diagnostics_emit(const char *category, const char *message, int write_stderr)
{
	char formatted[LCU_DIAGNOSTICS_MAX_MESSAGE];
	SYSTEMTIME now;
	int formatted_len;
	size_t message_len;
	int needs_newline;

	if (!category)
	{
		category = "DIAGNOSTIC";
	}
	if (!message)
	{
		message = "(null)";
	}

	GetLocalTime(&now);
	message_len = strlen(message);
	needs_newline = message_len == 0 || message[message_len - 1] != '\n';
	formatted_len = snprintf(formatted, sizeof(formatted),
		"[%04u-%02u-%02u %02u:%02u:%02u.%03u][pid=%lu][tid=%lu][%s] %s%s",
		now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond,
		now.wMilliseconds, GetCurrentProcessId(), GetCurrentThreadId(),
		category, message, needs_newline ? "\r\n" : "");
	if (formatted_len < 0)
	{
		return;
	}
	if ((size_t)formatted_len >= sizeof(formatted))
	{
		formatted_len = (int)sizeof(formatted) - 1;
		formatted[formatted_len - 2] = '\r';
		formatted[formatted_len - 1] = '\n';
	}

	if (write_stderr)
	{
		lcu_diagnostics_write_stderr(formatted, (size_t)formatted_len);
	}
	lcu_diagnostics_write_log(formatted, (size_t)formatted_len);
}

static void lcu_diagnostics_abort_now(void)
{
	_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
	abort();
	TerminateProcess(GetCurrentProcess(), 3);
}

#ifdef _DEBUG
static const char *lcu_diagnostics_report_category(int report_type)
{
	switch (report_type)
	{
	case _CRT_ASSERT:
		return "CRT_ASSERT";
	case _CRT_ERROR:
		return "CRT_ERROR";
	case _CRT_WARN:
		return "CRT_WARN";
	default:
		return "CRT";
	}
}

static int __cdecl lcu_diagnostics_crt_hook(int report_type, char *message, int *return_value)
{
	if (return_value)
	{
		*return_value = 0;
	}
	lcu_diagnostics_emit(lcu_diagnostics_report_category(report_type), message,
		report_type == _CRT_ASSERT);
	if (report_type == _CRT_ASSERT)
	{
		lcu_diagnostics_abort_now();
	}
	return 0;
}

static int __cdecl lcu_diagnostics_crt_hook_wide(int report_type, wchar_t *message, int *return_value)
{
	char utf8_message[LCU_DIAGNOSTICS_MAX_MESSAGE];
	int converted;

	if (return_value)
	{
		*return_value = 0;
	}
	converted = WideCharToMultiByte(CP_UTF8, 0, message ? message : L"(null)", -1,
		utf8_message, (int)sizeof(utf8_message), NULL, NULL);
	if (converted <= 0)
	{
		strcpy_s(utf8_message, sizeof(utf8_message), "(wide CRT report conversion failed)");
	}
	lcu_diagnostics_emit(lcu_diagnostics_report_category(report_type), utf8_message,
		report_type == _CRT_ASSERT);
	if (report_type == _CRT_ASSERT)
	{
		lcu_diagnostics_abort_now();
	}
	return 0;
}

static int lcu_diagnostics_crt_equal(const lcu_diagnostics_crt_api_t *left,
	const lcu_diagnostics_crt_api_t *right)
{
	return left->set_report_mode == right->set_report_mode;
}

static void lcu_diagnostics_configure_crt(lcu_diagnostics_crt_entry_t *entry)
{
	int debug_flags;
	lcu_diagnostics_crt_api_t *api = &entry->api;

	api->set_report_mode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	api->set_report_file(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
	api->set_report_mode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	api->set_report_file(_CRT_ERROR, _CRTDBG_FILE_STDERR);
	api->set_report_mode(_CRT_WARN, _CRTDBG_MODE_FILE);
	api->set_report_file(_CRT_WARN, _CRTDBG_FILE_STDERR);
	api->set_report_hook(_CRT_RPTHOOK_INSTALL, lcu_diagnostics_crt_hook);
	api->set_report_hook_wide(_CRT_RPTHOOK_INSTALL, lcu_diagnostics_crt_hook_wide);
	api->set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
	debug_flags = api->set_debug_flag(_CRTDBG_REPORT_FLAG);
	api->set_debug_flag(debug_flags | _CRTDBG_ALLOC_MEM_DF |
		_CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF);
}

static void lcu_diagnostics_release_crt(lcu_diagnostics_crt_entry_t *entry)
{
	int debug_flags;
	lcu_diagnostics_crt_api_t *api = &entry->api;

	debug_flags = api->set_debug_flag(_CRTDBG_REPORT_FLAG);
	api->set_debug_flag(debug_flags & ~_CRTDBG_LEAK_CHECK_DF);
	api->dump_memory_leaks();
	api->set_report_hook_wide(_CRT_RPTHOOK_REMOVE, lcu_diagnostics_crt_hook_wide);
	api->set_report_hook(_CRT_RPTHOOK_REMOVE, lcu_diagnostics_crt_hook);
}

static void lcu_diagnostics_register_crt_entry(const lcu_diagnostics_crt_api_t *crt_api)
{
	size_t i;
	for (i = 0; i < g_crt_count; ++i)
	{
		if (lcu_diagnostics_crt_equal(&g_crt_entries[i].api, crt_api))
		{
			++g_crt_entries[i].references;
			return;
		}
	}
	if (g_crt_count >= LCU_DIAGNOSTICS_MAX_CRTS)
	{
		lcu_diagnostics_emit("DIAGNOSTICS",
			"too many distinct CRT instances; report hook was not installed", 1);
		return;
	}
	g_crt_entries[g_crt_count].api = *crt_api;
	g_crt_entries[g_crt_count].references = 1;
	lcu_diagnostics_configure_crt(&g_crt_entries[g_crt_count]);
	++g_crt_count;
}

static void lcu_diagnostics_unregister_crt_entry(const lcu_diagnostics_crt_api_t *crt_api)
{
	size_t i;
	for (i = 0; i < g_crt_count; ++i)
	{
		if (lcu_diagnostics_crt_equal(&g_crt_entries[i].api, crt_api))
		{
			if (--g_crt_entries[i].references == 0)
			{
				lcu_diagnostics_release_crt(&g_crt_entries[i]);
				if (i + 1 < g_crt_count)
				{
					memmove(&g_crt_entries[i], &g_crt_entries[i + 1],
						(g_crt_count - i - 1) * sizeof(g_crt_entries[0]));
				}
				--g_crt_count;
			}
			return;
		}
	}
}

static lcu_diagnostics_crt_api_t lcu_diagnostics_local_crt_api(void)
{
	return lcu_diagnostics_current_crt_api();
}

void lcu_diagnostics_register_crt(const lcu_diagnostics_crt_api_t *crt_api)
{
	lcu_diagnostics_crt_api_t local_api;

	if (!crt_api || !crt_api->set_report_mode)
	{
		return;
	}
	lcu_diagnostics_ensure_sink();
	local_api = lcu_diagnostics_local_crt_api();

	AcquireSRWLockExclusive(&g_crt_lock);
	lcu_diagnostics_register_crt_entry(crt_api);
	if (!lcu_diagnostics_crt_equal(crt_api, &local_api))
	{
		lcu_diagnostics_register_crt_entry(&local_api);
	}
	ReleaseSRWLockExclusive(&g_crt_lock);
}

void lcu_diagnostics_unregister_crt(const lcu_diagnostics_crt_api_t *crt_api)
{
	lcu_diagnostics_crt_api_t local_api;

	if (!crt_api || !crt_api->set_report_mode)
	{
		return;
	}
	local_api = lcu_diagnostics_local_crt_api();

	AcquireSRWLockExclusive(&g_crt_lock);
	lcu_diagnostics_unregister_crt_entry(crt_api);
	if (!lcu_diagnostics_crt_equal(crt_api, &local_api))
	{
		lcu_diagnostics_unregister_crt_entry(&local_api);
	}
	ReleaseSRWLockExclusive(&g_crt_lock);
}
#endif // defined(_WIN32) && defined(_DEBUG)

void lcu_diagnostics_init(void)
{
	lcu_diagnostics_ensure_sink();
}

void lcu_diagnostics_deinit(void)
{
}

void lcu_diagnostics_write(const char *category, const char *message)
{
	lcu_diagnostics_emit(category, message, 1);
}

void lcu_diagnostics_writef(const char *category, const char *format, ...)
{
	char message[LCU_DIAGNOSTICS_MAX_MESSAGE];
	va_list args;
	int result;

	va_start(args, format);
	result = vsnprintf(message, sizeof(message),
		format ? format : "(null format)", args);
	va_end(args);
	if (result < 0)
	{
		strcpy_s(message, sizeof(message), "(diagnostic formatting failed)");
	}
	message[sizeof(message) - 1] = '\0';
	lcu_diagnostics_write(category, message);
}

void lcu_diagnostics_assert_fail(const char *expression, const char *function_name,
	const char *file_path, int file_line)
{
	char message[LCU_DIAGNOSTICS_MAX_MESSAGE];
	snprintf(message, sizeof(message),
		"ASSERT failed: '%s' at '%s' (%s:%d)",
		expression ? expression : "(null)",
		function_name ? function_name : "(unknown)",
		file_path ? file_path : "(unknown)", file_line);
	message[sizeof(message) - 1] = '\0';
	lcu_diagnostics_emit("ASSERT", message, 1);
	lcu_diagnostics_abort_now();
}

void lcu_diagnostics_fatalf(const char *category, const char *format, ...)
{
	char message[LCU_DIAGNOSTICS_MAX_MESSAGE];
	va_list args;
	int result;

	va_start(args, format);
	result = vsnprintf(message, sizeof(message),
		format ? format : "(null format)", args);
	va_end(args);
	if (result < 0)
	{
		strcpy_s(message, sizeof(message), "(diagnostic formatting failed)");
	}
	message[sizeof(message) - 1] = '\0';
	lcu_diagnostics_emit(category, message, 1);
	lcu_diagnostics_abort_now();
}

#else

void lcu_diagnostics_init(void)
{
}

void lcu_diagnostics_deinit(void)
{
}

void lcu_diagnostics_write(const char *category, const char *message)
{
	fprintf(stderr, "[%s] %s\n", category ? category : "DIAGNOSTIC",
		message ? message : "(null)");
	fflush(stderr);
}

void lcu_diagnostics_writef(const char *category, const char *format, ...)
{
	va_list args;
	fprintf(stderr, "[%s] ", category ? category : "DIAGNOSTIC");
	va_start(args, format);
	vfprintf(stderr, format ? format : "(null format)", args);
	va_end(args);
	fputc('\n', stderr);
	fflush(stderr);
}

void lcu_diagnostics_assert_fail(const char *expression, const char *function_name,
	const char *file_path, int file_line)
{
	lcu_diagnostics_writef("ASSERT", "ASSERT failed: '%s' at '%s' (%s:%d)",
		expression ? expression : "(null)",
		function_name ? function_name : "(unknown)",
		file_path ? file_path : "(unknown)", file_line);
	abort();
}

void lcu_diagnostics_fatalf(const char *category, const char *format, ...)
{
	va_list args;
	fprintf(stderr, "[%s] ", category ? category : "FATAL");
	va_start(args, format);
	vfprintf(stderr, format ? format : "(null format)", args);
	va_end(args);
	fputc('\n', stderr);
	fflush(stderr);
	abort();
}

#endif // !_WIN32
