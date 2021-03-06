#pragma once
#ifndef __PTHREAD_WINDOWS_H
#define __PTHREAD_WINDOWS_H

#include "config/lcu_build_config.h"

//do not include this header directly, you should include "thread_wrapper.h" instead!
#if(defined(_WIN32) && _LCU_CFG_WIN_PTHREAD_MODE == LCU_WIN_PTHREAD_IMPLEMENT_MODE_SIMPLE)

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
* pthread functions
*/
typedef struct pthread_t pthread_t;
typedef int pthread_attr_t;

typedef CRITICAL_SECTION pthread_mutex_t;
typedef int pthread_mutexattr_t;

typedef struct pthread_cond_t pthread_cond_t;
typedef int pthread_condattr_t;

struct pthread_t
{
	HANDLE mThreadHandle;
	DWORD mThreadID;
	void* (*mStart)(void*);
	void* mArgs;
	void* mExitCode;
};

struct pthread_cond_t
{
	pthread_mutex_t mLock;
	size_t mWaiting;
	size_t mWake;
	size_t mGeneration;
	HANDLE mSemaphore;
};

inline unsigned int pthread_self()
{
	return GetCurrentThreadId();
}

int pthread_create(pthread_t* tid, const pthread_attr_t* attr, void* (*start)(void*), void* arg);
int pthread_join(pthread_t tid, void** value_ptr);

/*
*Mutex Functions
*/
int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr);
int pthread_mutex_destroy(pthread_mutex_t* mutex);
int pthread_mutex_lock(pthread_mutex_t* mutex);
int pthread_mutex_unlock(pthread_mutex_t* mutex);

/*
*Condition Variable Functions
*/
int pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t* attr);
int pthread_cond_destroy(pthread_cond_t* cond);
int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex);
int pthread_cond_signal(pthread_cond_t* cond);
int pthread_cond_broadcast(pthread_cond_t* cond);

#ifdef __cplusplus
};
#endif // __cplusplus

#endif 

#endif //__PTHREAD_WINDOWS_H