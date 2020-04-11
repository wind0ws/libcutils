#pragma once
#ifndef __COMMON_THREAD_WRAPPER_H
#define __COMMON_THREAD_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32

#include "pthread_windows.h"
#include "semaphore_windows.h"

#define usleep(micro_seconds)    Sleep((micro_seconds) / 1000)
#define sleep(seconds) Sleep((seconds) * 1000)

#else /* for unix */
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#define Sleep(millseconds) usleep((millseconds) * 1000)
#endif /* #ifdef _WIN32 */

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
// No definition needed for Android because we'll just pick up bionic's copy.
#ifndef __ANDROID__
#ifndef __pid_t_defined
	typedef int __pid_t;
	typedef __pid_t pid_t;
# define __pid_t_defined
#endif
	pid_t gettid();
#endif  // __ANDROID__

	unsigned int pthread_id(pthread_t* pth);

#ifdef __cplusplus
};
#endif

#endif /* #ifdef __COMMON_THREAD_WRAPPER_H*/
