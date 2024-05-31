/**
 * @file portable_thread.h
 * @brief interface for thread/mutex/rwlock/semaphore operation.
 * @version 1.0
 * @date 2024-01-03
 * 
 */
#pragma once
#ifndef PORTABLE_THREAD_H
#define PORTABLE_THREAD_H

#if(defined(Sleep) || defined(usleep))
// nothing need define
#elif(defined(PLATFORM_HENGXUAN) || defined(CMSIS_OS_VER))
#include "cmsis_os2.h"
#define Sleep(ms)             osDelay(ms)
#define usleep(micro_seconds) Sleep((micro_seconds) / 1000) 
#define sleep(seconds)        Sleep((seconds) * 1000)

#elif(defined(_WIN32))
#pragma warning(push)
#pragma warning(disable: 5105)

#include <windows.h>

#ifndef usleep
#define usleep(micro_seconds)    Sleep((micro_seconds) / 1000)
#endif // !usleep
#ifndef sleep
#define sleep(seconds)           Sleep((seconds) * 1000)
#endif // !sleep

#pragma warning(pop)

#else /* for unix */

#include <unistd.h>
#ifndef Sleep
#define Sleep(millseconds)       usleep((millseconds) * 1000)
#endif // !Sleep

#endif // Sleep

#ifndef GETTID
#define GETTID()   portable_thread_gettid()
#endif // !GETTID
#ifndef THREAD_SETNAME_FOR_CURRENT
#define THREAD_SETNAME_FOR_CURRENT(name) portable_thread_set_current_name(name)
#endif // !THREAD_SETNAME_FOR_CURRENT

typedef struct
{
	char name[32];
	unsigned int stack_size;
	void* platform_attr;
} portable_thread_attr_t;

typedef void *portable_thread_t;
typedef void *portable_mutex_t;
typedef void *portable_rwlock_t;
typedef void *portable_sem_t;

#ifdef __cplusplus
extern "C"
{
#endif

    // get current tid. you can cast it to pid_t
    unsigned long portable_thread_gettid();

	// set name for current thread. suggest use this method instead of pthread_setname_np
	int portable_thread_set_current_name(const char* name);

    // create and start thread
    int portable_thread_create(portable_thread_t *newthread_p,
                           const portable_thread_attr_t *thread_attr,
                           void *(*start_routine)(void *),
                           void *arg);
    // join thread until it exit
    int portable_thread_join(portable_thread_t th, void **thread_return);


    // create mutex
    int portable_mutex_init(portable_mutex_t *mutex, void *mutex_attr);
    // lock mutex
    int portable_mutex_lock(portable_mutex_t *mutex);
    // unlock mutex
    int portable_mutex_unlock(portable_mutex_t *mutex);
    // delete(destroy) mutex
    int portable_mutex_destroy(portable_mutex_t *mutex);


    // init rwlock
	int portable_rwlock_init(portable_rwlock_t* lock, const void* attr);
    // delete(destroy) rwlock
    int portable_rwlock_destroy(portable_rwlock_t* lock);
    // acquire read lock
    int portable_rwlock_rdlock(portable_rwlock_t* lock);
    // acquire write lock
    int portable_rwlock_wrlock(portable_rwlock_t* lock);
    // unlock read/write lock
    int portable_rwlock_unlock(portable_rwlock_t* lock);


    // init sem
	int portable_sem_init(portable_sem_t* sem, int pshared, unsigned int value);
    // wait sem
    int portable_sem_wait(portable_sem_t* sem);
    // post sem
    int portable_sem_post(portable_sem_t* sem);
    // get sem value
    int portable_sem_getvalue(portable_sem_t* sem, int* sval);
    // destroy sem
    int portable_sem_destroy(portable_sem_t* sem);
    

#ifdef __cplusplus
}
#endif

#endif // ！PORTABLE_THREAD_H
