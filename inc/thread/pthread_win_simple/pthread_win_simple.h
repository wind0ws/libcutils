#pragma once
#ifndef PTHREAD_WINDOWS_H
#define PTHREAD_WINDOWS_H

#include "config/lcu_build_config.h"

//do not include this header directly, you should include "posix_thread.h" instead!
#if(defined(_WIN32) && _LCU_CFG_WIN_PTHREAD_MODE == LCU_WIN_PTHREAD_IMPLEMENT_MODE_SIMPLE)
#pragma warning(push)
#pragma warning(disable: 5105)
#include <windows.h>
#include <stdbool.h>
#include <stddef.h>
#pragma warning(pop)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pthread_s
{
	volatile DWORD mThreadID;
	HANDLE mThreadHandle;
	void* (*mStart)(void*);
	void* mArgs;
	void* mExitCode;
	volatile bool mDetached;
} *pthread_t;
typedef int pthread_attr_t;

typedef struct pthread_mutex_s
{
	CRITICAL_SECTION mHandle;
} *pthread_mutex_t;

typedef int pthread_mutexattr_t;


typedef struct pthread_cond_t pthread_cond_t;
typedef int pthread_condattr_t;
struct pthread_cond_t
{
	pthread_mutex_t mLock;
	size_t mWaiting;
	size_t mWake;
	size_t mGeneration;
	HANDLE mSemaphore;
};

typedef int pthread_rwlockattr_t;
typedef struct
{
	HANDLE   m_mutex;
	HANDLE   m_readEvent;
	HANDLE   m_writeEvent;
	size_t   m_readers;
	size_t   m_writersWaiting;
	size_t   m_writers;
} *pthread_rwlock_t;

/*
 * ====================
 * ====================
 * Object initializers
 * ====================
 * ====================
 */
#define PTHREAD_MUTEX_INITIALIZER            ((pthread_mutex_t)(size_t) -1)
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER  ((pthread_mutex_t)(size_t) -2)
#define PTHREAD_ERRORCHECK_MUTEX_INITIALIZER ((pthread_mutex_t)(size_t) -3)

 /*
  * Compatibility with LinuxThreads
  */
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP   PTHREAD_RECURSIVE_MUTEX_INITIALIZER
#define PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP  PTHREAD_ERRORCHECK_MUTEX_INITIALIZER

#define PTHREAD_COND_INITIALIZER     ((pthread_cond_t)(size_t) -1)

#define PTHREAD_RWLOCK_INITIALIZER   ((pthread_rwlock_t)(size_t) -1)

#define PTHREAD_SPINLOCK_INITIALIZER ((pthread_spinlock_t)(size_t) -1)


//=================== pthread functions	 ===========================
// not implemented, this will return NULL.
pthread_t pthread_self();
int pthread_create(pthread_t* tid_p, const pthread_attr_t* attr, void* (*start)(void*), void* arg);
int pthread_join(pthread_t tid, void** value_ptr);
int pthread_detach(pthread_t tid);

/*
 * Mutex Functions
 */
int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr);
int pthread_mutex_destroy(pthread_mutex_t* mutex);
int pthread_mutex_lock(pthread_mutex_t* mutex);
int pthread_mutex_unlock(pthread_mutex_t* mutex);

/*
 * Condition Variable Functions
 */
int pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t* attr);
int pthread_cond_destroy(pthread_cond_t* cond);
int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex);
int pthread_cond_signal(pthread_cond_t* cond);
int pthread_cond_broadcast(pthread_cond_t* cond);

/**
 * RW Locks
 */
int pthread_rwlock_init(pthread_rwlock_t* rwlock, const pthread_rwlockattr_t* attr);
int pthread_rwlock_destroy(pthread_rwlock_t* rwlock);
int pthread_rwlock_rdlock(pthread_rwlock_t* rwlock);
int pthread_rwlock_wrlock(pthread_rwlock_t* rwlock);
int pthread_rwlock_unlock(pthread_rwlock_t* rwlock);

#ifdef __cplusplus
};
#endif // __cplusplus

#endif // _WIN32 && _LCU_CFG_WIN_PTHREAD_MODE == LCU_WIN_PTHREAD_IMPLEMENT_MODE_SIMPLE

#endif // !PTHREAD_WINDOWS_H
