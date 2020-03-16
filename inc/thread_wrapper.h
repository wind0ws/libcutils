#ifndef __COMMON_THREAD_WRAPPER_H
#define __COMMON_THREAD_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32

#include "pthread_windows.h"
#include "semaphore_windows.h"

#define usleep(useconds)    Sleep(useconds / 1000)

#else /* for unix */
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#endif /* #ifdef _WIN32 */

	unsigned int pthread_id(pthread_t* pth);

#ifdef __cplusplus
};
#endif

#endif /* #ifdef __COMMON_THREAD_WRAPPER_H*/
