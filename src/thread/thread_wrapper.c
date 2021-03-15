#include "thread/thread_wrapper.h"

#ifdef _WIN32
//reference: https://docs.microsoft.com/en-us/visualstudio/debugger/how-to-set-a-thread-name-in-native-code?view=vs-2019
#include <windows.h>
const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // Must be 0x1000.
	LPCSTR szName; // Pointer to name (in user addr space).
	DWORD dwThreadID; // Thread ID (-1=caller thread).
	DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)
//The SetThreadName function shown below demonstrates this exception-based approach. 
//Note that the thread name will be automatically copied to the thread, 
//so that the memory for the threadName parameter can be released after the SetThreadName call is completed.
static void SetThreadName(DWORD dwThreadID, const char* threadName) 
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
#if _LCU_CFG_WIN_PTHREAD_MODE == LCU_WIN_PTHREAD_IMPLEMENT_MODE_LIB
	DWORD thr_id = pthread_getw32threadid_np(thr);
#elif _LCU_CFG_WIN_PTHREAD_MODE == LCU_WIN_PTHREAD_IMPLEMENT_MODE_SIMPLE
	DWORD thr_id = thr.mThreadID;
#else
#error " unknow _LCU_CFG_WIN32_PTHREAD_MODE "
#endif // _LCU_CFG_WIN_PTHREAD_MODE
	SetThreadName(thr_id, name);
	return 0;
}
#endif // _WIN32

#ifndef __ANDROID__ // Android just pick up bionic's copy.
pid_t gettid() 
{
	#if defined(__APPLE__)
		return syscall(SYS_thread_selfid);
	#elif defined(__linux__)
		return syscall(__NR_gettid);
	#elif defined(_WIN32)
		return GetCurrentThreadId();
	#endif
}
#endif  // __ANDROID__
