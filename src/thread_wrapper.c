//
// Created by Admin on 2020/2/12.
//
#ifdef _WIN32

#ifdef __cplusplus
extern "C"
{
#endif// __cplusplus

#include "thread_wrapper.h"

#ifndef __ANDROID__
#ifndef __pid_t_defined
	typedef int __pid_t;
	typedef __pid_t pid_t;
# define __pid_t_defined
#endif
	pid_t gettid() {
#if defined(__APPLE__)
		return syscall(SYS_thread_selfid);
#elif defined(__linux__)
		return syscall(__NR_gettid);
#elif defined(_WIN32)
		return GetCurrentThreadId();
#endif
	}
#endif  // __ANDROID__
	
    unsigned int pthread_id(pthread_t *pth) 
    {
#ifdef WIN32
        return (unsigned int)(pth->mThreadID);
#else
        return (unsigned int)(*pth);
#endif // WIN32
    }

#ifdef __cplusplus
};
#endif // __cplusplus

#endif /* #ifdef _WIN32 */
