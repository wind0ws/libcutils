#pragma once
#ifndef LCU_POSIX_THREAD_H
#define LCU_POSIX_THREAD_H

#include "config/lcu_build_config.h"

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable: 5105)

#if _LCU_CFG_WIN_PTHREAD_MODE == LCU_WIN_PTHREAD_IMPLEMENT_MODE_LIB         /* use posix-win32 lib          */
#include "thread/pthread_win_lib/pthread_win_lib.h"
#include "thread/pthread_win_lib/sched_win_lib.h"
#include "thread/pthread_win_lib/semaphore_win_lib.h"
#elif _LCU_CFG_WIN_PTHREAD_MODE == LCU_WIN_PTHREAD_IMPLEMENT_MODE_SIMPLE    /* use windows native implement */
#include "thread/pthread_win_simple/pthread_win_simple.h"
#include "thread/pthread_win_simple/semaphore_win_simple.h"
#else
#error " unknow _LCU_CFG_WIN32_PTHREAD_MODE "
#endif // _LCU_CFG_WIN_PTHREAD_MODE

#ifndef usleep
#define usleep(micro_seconds)    Sleep((micro_seconds) / 1000)
#endif // !usleep
#ifndef sleep
#define sleep(seconds)           Sleep((seconds) * 1000)
#endif // !sleep

#pragma warning(pop)

#else /* for unix */

#ifndef _GNU_SOURCE // for enable pthread_setname_np
#define _GNU_SOURCE
#endif // !_GNU_SOURCE
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#ifndef Sleep
#define Sleep(millseconds)       usleep((millseconds) * 1000)
#endif // !Sleep

#endif // _WIN32

// For gettid.
#if defined(__APPLE__)
#include "AvailabilityMacros.h"  // For MAC_OS_X_VERSION_MAX_ALLOWED
#include <stdint.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>
#elif(defined(__linux__) || defined(__ANDROID__))
#include <sys/prctl.h> /* for prctl */
#include <syscall.h>
#include <unistd.h>
#elif defined(_WIN32)
#pragma warning(push)
#pragma warning(disable: 5105)
#include <windows.h>
#pragma warning(pop)
#endif

/* macro for gettid */
#if defined(__APPLE__)
#define GETTID()  syscall(SYS_thread_selfid)
#elif defined(__ANDROID__) // Android just pick up bionic's copy.
#define GETTID()  gettid()
#elif defined(__linux__)
#define GETTID()  syscall(__NR_gettid)
#elif defined(_WIN32)
#define GETTID()  GetCurrentThreadId()
#else
#error should implement macro of GETTID()
#endif

#define THREAD_SETNAME(thr, name)        pthread_setname_np(thr, name)
#define THREAD_SETNAME_FOR_CURRENT(name) posix_thread_set_current_name(name)

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
	int pthread_setname_np(pthread_t thr, const char* name);
#endif // _WIN32

	// set name for current thread. suggest use this method instead of pthread_setname_np
	int posix_thread_set_current_name(const char* name);

#ifdef __cplusplus
};
#endif

#endif // !LCU_POSIX_THREAD_H
