#include "thread/posix_thread.h"

#ifdef _WIN32
//reference: https://docs.microsoft.com/en-us/visualstudio/debugger/how-to-set-a-thread-name-in-native-code
#pragma warning(push)
#pragma warning(disable: 5105)
#include <windows.h>
#pragma warning(pop)

#define MS_VC_EXCEPTION  (DWORD)0x406D1388
#pragma pack(push, 8)
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // Must be 0x1000.
	LPCSTR szName; // Pointer to name (in user addr space).
	DWORD dwThreadID; // Thread ID (-1=caller thread).
	DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

//The SetWin32ThreadName function shown below demonstrates this exception-based approach. 
//Note that the thread name will be automatically copied to the thread, 
//so that the memory for the threadName parameter can be released after the SetWin32ThreadName call is completed.
static void pri_set_win32_thread_name(DWORD dwThreadID, const char* threadName) 
{
	THREADNAME_INFO info = {0};
	info.dwType = 0x1000;
	info.szName = threadName;
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
	__try 
	{
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) 
	{
	}
#pragma warning(pop)
}

int pthread_setname_np(pthread_t thr, const char* name)
{
	if (NULL == name || '\0' == name[0])
	{
		return -1;
    }
#if _LCU_CFG_WIN_PTHREAD_MODE == LCU_WIN_PTHREAD_IMPLEMENT_MODE_LIB
	DWORD thr_id = pthread_getw32threadid_np(thr);
#elif _LCU_CFG_WIN_PTHREAD_MODE == LCU_WIN_PTHREAD_IMPLEMENT_MODE_SIMPLE
	DWORD thr_id = thr ? thr->mThreadID : GetCurrentThreadId();
#else
#error " unknow _LCU_CFG_WIN32_PTHREAD_MODE "
#endif // _LCU_CFG_WIN_PTHREAD_MODE
	pri_set_win32_thread_name(thr_id, name);
	return 0;
}
#endif // _WIN32

int posix_thread_set_current_name(const char* name)
{
	int ret = -1;
	if (NULL == name || '\0' == name[0])
	{
		return ret;
	}
#if(defined(HAVE_PTHREAD_SETNAME_NP) && HAVE_PTHREAD_SETNAME_NP)
	ret = pthread_setname_np(pthread_self(), name);
#elif(defined(__linux__))
	/* Use prctl (<sys/prctl.h>) instead to prevent using _GNU_SOURCE flag and implicit declaration */
	ret = prctl(PR_SET_NAME, name);
#elif(defined(__APPLE__) && defined(__MACH__))
	ret = pthread_setname_np(name);
#else
    #pragma message("posix_thread_set_current_name(): pthread_setname_np is not supported on this os")
#endif // HAVE_PTHREAD_SETNAME_NP
	return ret;
}
