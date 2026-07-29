#include "mem/mem_debug.h"
#include "common_macro.h"
#include "log/xlog.h"
#include "lcu_test_registry.h"
#include "thread/portable_thread.h"

#if defined(_WIN32) && defined(LCU_ENABLE_DLL_LIFECYCLE_TEST)

#include <windows.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#define XLOG_DLL_LIFECYCLE_ITERATIONS (10)
#define XLOG_DLL_LIFECYCLE_THREAD_COUNT (8)
#define XLOG_DLL_LIFECYCLE_THREAD_WAIT_MS (10000U)

typedef int (*xlog_global_lifecycle_fn)(void);
typedef void (*xlog_set_min_level_fn)(LogLevel min_level);
typedef int (*lcu_global_lifecycle_fn)(void);
typedef int (*portable_thread_create_fn)(portable_thread_t *newthread_p,
	const portable_thread_attr_t *thread_attr,
	void *(*start_routine)(void *), void *arg);
typedef int (*portable_thread_join_fn)(portable_thread_t thread, void **thread_return);

typedef struct
{
	volatile LONG *ready_count_p;
	volatile LONG *start_flag_p;
	xlog_global_lifecycle_fn global_init;
	int init_ret;
} xlog_init_thread_context_t;

static DWORD WINAPI pri_xlog_init_thread(void *arg)
{
	xlog_init_thread_context_t *context = (xlog_init_thread_context_t *)arg;
	InterlockedIncrement(context->ready_count_p);
	while (0 == InterlockedCompareExchange(context->start_flag_p, 0, 0))
	{
		Sleep(0);
	}
	context->init_ret = context->global_init();
	return 0;
}

static void *pri_portable_thread_return_arg(void *arg)
{
	return arg;
}

static int pri_run_portable_thread_join(portable_thread_create_fn thread_create,
	portable_thread_join_fn thread_join)
{
	portable_thread_t thread = NULL;
	void *const expected_return = (void *)(size_t)0x1234U;
	void *thread_return = NULL;
	const int create_ret = thread_create(&thread, NULL, pri_portable_thread_return_arg, expected_return);
	if (0 != create_ret)
	{
		fprintf(stderr, "xlog DLL lifecycle: portable_thread_create returned %d\n", create_ret);
		return -1;
	}

	const int join_ret = thread_join(thread, &thread_return);
	if (0 != join_ret || expected_return != thread_return)
	{
		fprintf(stderr, "xlog DLL lifecycle: portable_thread_join returned %d, expected=%p actual=%p\n",
			join_ret, expected_return, thread_return);
		return -1;
	}
	return 0;
}

static int pri_wait_threads_ready(volatile LONG *ready_count_p, LONG expected_count)
{
	const DWORD start_tick = GetTickCount();
	while (InterlockedCompareExchange(ready_count_p, 0, 0) < expected_count)
	{
		if ((DWORD)(GetTickCount() - start_tick) >= XLOG_DLL_LIFECYCLE_THREAD_WAIT_MS)
		{
			fprintf(stderr, "xlog DLL lifecycle: timed out waiting for %ld init threads\n",
				expected_count);
			return -1;
		}
		Sleep(1);
	}
	return 0;
}

static int pri_run_concurrent_xlog_init(xlog_global_lifecycle_fn global_init)
{
	HANDLE threads[XLOG_DLL_LIFECYCLE_THREAD_COUNT] = {0};
	xlog_init_thread_context_t contexts[XLOG_DLL_LIFECYCLE_THREAD_COUNT];
	volatile LONG ready_count = 0;
	volatile LONG start_flag = 0;
	int created_count = 0;
	int status = 0;
	memset(contexts, 0, sizeof(contexts));

	for (int i = 0; i < XLOG_DLL_LIFECYCLE_THREAD_COUNT; ++i)
	{
		contexts[i].ready_count_p = &ready_count;
		contexts[i].start_flag_p = &start_flag;
		contexts[i].global_init = global_init;
		contexts[i].init_ret = INT_MIN;
		threads[i] = CreateThread(NULL, 0, pri_xlog_init_thread, &contexts[i], 0, NULL);
		if (NULL == threads[i])
		{
			fprintf(stderr, "xlog DLL lifecycle: CreateThread failed at index=%d, error=%lu\n",
				i, GetLastError());
			status = -1;
			break;
		}
		++created_count;
	}

	if (0 != pri_wait_threads_ready(&ready_count, (LONG)created_count))
	{
		status = -1;
	}
	InterlockedExchange(&start_flag, 1);

	if (created_count > 0)
	{
		const DWORD wait_ret = WaitForMultipleObjects(
			(DWORD)created_count, threads, TRUE, XLOG_DLL_LIFECYCLE_THREAD_WAIT_MS);
		if (WAIT_OBJECT_0 != wait_ret)
		{
			fprintf(stderr, "xlog DLL lifecycle: WaitForMultipleObjects failed, result=%lu, error=%lu\n",
				wait_ret, GetLastError());
			status = -1;
			(void)WaitForMultipleObjects((DWORD)created_count, threads, TRUE, INFINITE);
		}
	}

	int first_init_count = 0;
	for (int i = 0; i < created_count; ++i)
	{
		DWORD exit_code = STILL_ACTIVE;
		if (!GetExitCodeThread(threads[i], &exit_code) || 0 != exit_code)
		{
			fprintf(stderr, "xlog DLL lifecycle: init thread=%d exit query/result failed, exit=%lu, error=%lu\n",
				i, exit_code, GetLastError());
			status = -1;
		}
		if (!CloseHandle(threads[i]))
		{
			fprintf(stderr, "xlog DLL lifecycle: CloseHandle failed for thread=%d, error=%lu\n",
				i, GetLastError());
			status = -1;
		}
		if (0 == contexts[i].init_ret)
		{
			++first_init_count;
		}
		else if (1 != contexts[i].init_ret)
		{
			fprintf(stderr, "xlog DLL lifecycle: init thread=%d returned %d\n",
				i, contexts[i].init_ret);
			status = -1;
		}
	}

	if (XLOG_DLL_LIFECYCLE_THREAD_COUNT != created_count || 1 != first_init_count)
	{
		fprintf(stderr, "xlog DLL lifecycle: expected one first init from %d threads, created=%d first=%d\n",
			XLOG_DLL_LIFECYCLE_THREAD_COUNT, created_count, first_init_count);
		status = -1;
	}
	return status;
}

static int pri_validate_lcu_dll_unloaded(HMODULE module, const char *stage)
{
	HMODULE retained_module = NULL;
	SetLastError(ERROR_SUCCESS);
	if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
						 GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
						 (LPCSTR)(const void *)module, &retained_module))
	{
		fprintf(stderr, "xlog DLL lifecycle: module still loaded after %s, expected=%p actual=%p\n",
			stage, (void *)module, (void *)retained_module);
		return -1;
	}
	const DWORD error_code = GetLastError();
	if (ERROR_MOD_NOT_FOUND != error_code)
	{
		fprintf(stderr, "xlog DLL lifecycle: module unload verification failed after %s, error=%lu\n",
			stage, error_code);
		return -1;
	}
	return 0;
}

static int pri_unload_lcu_dll(HMODULE module, const char *stage)
{
	if (!FreeLibrary(module))
	{
		fprintf(stderr, "xlog DLL lifecycle: FreeLibrary failed after %s, error=%lu\n",
			stage, GetLastError());
		return -1;
	}
	return pri_validate_lcu_dll_unloaded(module, stage);
}

static int pri_get_lcu_dll_path(char *dll_path, size_t dll_path_size)
{
	DWORD path_len = GetModuleFileNameA(NULL, dll_path, (DWORD)dll_path_size);
	if (0 == path_len || path_len >= dll_path_size)
	{
		fprintf(stderr, "xlog DLL lifecycle: GetModuleFileNameA failed, error=%lu\n", GetLastError());
		return -1;
	}

	char *last_separator = strrchr(dll_path, '\\');
	if (NULL == last_separator)
	{
		fprintf(stderr, "xlog DLL lifecycle: cannot build lcu.dll path from '%s'\n", dll_path);
		return -1;
	}
	const size_t directory_size = (size_t)(last_separator - dll_path) + 1U;
	if (directory_size > dll_path_size || sizeof("lcu.dll") > dll_path_size - directory_size)
	{
		fprintf(stderr, "xlog DLL lifecycle: lcu.dll path exceeds buffer, size=%zu\n", dll_path_size);
		return -1;
	}

	memcpy(last_separator + 1, "lcu.dll", sizeof("lcu.dll"));
	return 0;
}

static int pri_run_lcu_dll_lifecycle(const char *dll_path)
{
	HMODULE module = LoadLibraryA(dll_path);
	if (NULL == module)
	{
		fprintf(stderr, "xlog DLL lifecycle: LoadLibraryA('%s') failed, error=%lu\n", dll_path, GetLastError());
		return -1;
	}

	xlog_global_lifecycle_fn global_init = (xlog_global_lifecycle_fn)GetProcAddress(module, "xlog_global_init");
	xlog_global_lifecycle_fn global_cleanup = (xlog_global_lifecycle_fn)GetProcAddress(module, "xlog_global_cleanup");
	xlog_set_min_level_fn set_min_level = (xlog_set_min_level_fn)GetProcAddress(module, "xlog_set_min_level");
	lcu_global_lifecycle_fn lcu_init = (lcu_global_lifecycle_fn)GetProcAddress(module, "lcu_global_init");
	lcu_global_lifecycle_fn lcu_cleanup = (lcu_global_lifecycle_fn)GetProcAddress(module, "lcu_global_cleanup");
	portable_thread_create_fn thread_create =
		(portable_thread_create_fn)GetProcAddress(module, "portable_thread_create");
	portable_thread_join_fn thread_join =
		(portable_thread_join_fn)GetProcAddress(module, "portable_thread_join");
	if (NULL == global_init || NULL == global_cleanup || NULL == set_min_level ||
		NULL == lcu_init || NULL == lcu_cleanup || NULL == thread_create || NULL == thread_join)
	{
		fprintf(stderr, "xlog DLL lifecycle: GetProcAddress failed, error=%lu\n", GetLastError());
		(void)pri_unload_lcu_dll(module, "GetProcAddress failure");
		return -1;
	}

	const int init_ret = pri_run_concurrent_xlog_init(global_init);
	set_min_level(LOG_LEVEL_VERBOSE);
	const int cleanup_ret = global_cleanup();
	if (0 != init_ret || 0 != cleanup_ret)
	{
		fprintf(stderr, "xlog DLL lifecycle: concurrent init returned %d, cleanup returned %d\n",
			init_ret, cleanup_ret);
		(void)pri_unload_lcu_dll(module, "xlog lifecycle failure");
		return -1;
	}

	const int lcu_init_ret = lcu_init();
	const int thread_join_ret = (0 == lcu_init_ret) ?
		pri_run_portable_thread_join(thread_create, thread_join) : -1;
	const int lcu_cleanup_ret = lcu_cleanup();
	if (0 != lcu_init_ret || 0 != thread_join_ret || 0 != lcu_cleanup_ret)
	{
		fprintf(stderr,
			"xlog DLL lifecycle: lcu init returned %d, thread join returned %d, cleanup returned %d\n",
			lcu_init_ret, thread_join_ret, lcu_cleanup_ret);
		(void)pri_unload_lcu_dll(module, "lcu lifecycle failure");
		return -1;
	}

	const int lcu_reinit_ret = lcu_init();
	const int thread_rejoin_ret = (0 == lcu_reinit_ret) ?
		pri_run_portable_thread_join(thread_create, thread_join) : -1;
	const int lcu_recleanup_ret = lcu_cleanup();
	if (0 != lcu_reinit_ret || 0 != thread_rejoin_ret || 0 != lcu_recleanup_ret)
	{
		fprintf(stderr,
			"xlog DLL lifecycle: lcu reinit returned %d, thread rejoin returned %d, recleanup returned %d\n",
			lcu_reinit_ret, thread_rejoin_ret, lcu_recleanup_ret);
		(void)pri_unload_lcu_dll(module, "lcu reinit lifecycle failure");
		return -1;
	}

	if (0 != pri_unload_lcu_dll(module, "successful cleanup"))
	{
		return -1;
	}

	return 0;
}

int lcu_dll_lifecycle_test(void)
{
	char dll_path[MAX_PATH] = {0};
	if (0 != pri_get_lcu_dll_path(dll_path, sizeof(dll_path)))
	{
		return -1;
	}

	if (0 != pri_run_lcu_dll_lifecycle(dll_path))
	{
		return -1;
	}

	DWORD baseline_handle_count = 0;
	if (!GetProcessHandleCount(GetCurrentProcess(), &baseline_handle_count))
	{
		fprintf(stderr, "xlog DLL lifecycle: GetProcessHandleCount failed, error=%lu\n", GetLastError());
		return -1;
	}

	for (int i = 0; i < XLOG_DLL_LIFECYCLE_ITERATIONS; ++i)
	{
		if (0 != pri_run_lcu_dll_lifecycle(dll_path))
		{
			return -1;
		}

		DWORD handle_count = 0;
		if (!GetProcessHandleCount(GetCurrentProcess(), &handle_count))
		{
			fprintf(stderr, "xlog DLL lifecycle: GetProcessHandleCount failed, error=%lu\n", GetLastError());
			return -1;
		}
		if (handle_count != baseline_handle_count)
		{
			fprintf(stderr, "xlog DLL lifecycle: iteration=%d expected handles=%lu actual=%lu\n",
				i + 1, baseline_handle_count, handle_count);
			return -1;
		}
	}

	return 0;
}

LCU_TEST_REGISTER(lcu_dll_lifecycle_test, "reload lcu.dll without process handle growth");

#endif
