#include "thread/thread_wrapper.h"
#include "lcu_build_config.h"

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

