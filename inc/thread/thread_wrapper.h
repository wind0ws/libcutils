#pragma once
#ifndef LCU_THREAD_WRAPPER_H
#define LCU_THREAD_WRAPPER_H

#include "config/lcu_build_config.h"

#ifdef _WIN32
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
#elif defined(__linux__) && !defined(__ANDROID__)
#include <syscall.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
	int pthread_setname_np(pthread_t thr, const char* name);
#endif // _WIN32

	// set name for pthread. suggest use this method instead of pthread_setname_np
	int pthread_set_name(pthread_t thr, const char* name);

	// No definition needed for Android because we'll just pick up bionic's copy.
#ifndef __ANDROID__
#ifndef __pid_t_defined
	typedef int __pid_t;
	typedef __pid_t pid_t;
#define __pid_t_defined
#endif // !__pid_t_defined
	pid_t gettid();
#endif  // __ANDROID__

#ifdef __cplusplus
};
#endif

#endif // !LCU_THREAD_WRAPPER_H
