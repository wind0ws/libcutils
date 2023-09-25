#include "thread/pthread_win_simple/pthread_win_simple.h"

#if(defined(_WIN32) && _LCU_CFG_WIN_PTHREAD_MODE == LCU_WIN_PTHREAD_IMPLEMENT_MODE_SIMPLE)

#include "thread/pthread_win_simple/ptw32_mcs_lock.h"
#include "thread/pthread_win_simple/ptw32_implement.h"

#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#define CHK_HANDLE(hdl) do { if ( NULL == (hdl)) { return EINVAL; } } while (0)

static DWORD WINAPI pri_win_thread_start_routine(LPVOID lpParam);

int pthread_create(pthread_t* tid_p, const pthread_attr_t* attr, void* (*start)(void*), void* arg)
{
    *tid_p = NULL;
    pthread_t pth = (pthread_t)malloc(sizeof(struct pthread_s));
	CHK_HANDLE(pth);
    memset(pth, 0, sizeof(struct pthread_s));
	pth->mStart = start;
    pth->mArgs = arg;
    pth->mThreadHandle = CreateThread(NULL, 0, pri_win_thread_start_routine, 
		(LPVOID)pth, CREATE_SUSPENDED, (LPDWORD) &pth->mThreadID);
    if (NULL == pth->mThreadHandle) 
	{
        free(pth);
        return -1;
	}
    ResumeThread(pth->mThreadHandle);
    *tid_p = pth;
	return 0;
}

int pthread_join(pthread_t tid, void** value_ptr)
{
    if (NULL == tid || tid->mDetached) 
	{
        return EINVAL;
    }
	if (value_ptr)
	{
        *value_ptr = NULL;
	}
	if (WaitForSingleObject(tid->mThreadHandle, INFINITE) != WAIT_FAILED)
	{
        CloseHandle(tid->mThreadHandle);
        tid->mThreadHandle = NULL;
		if (value_ptr)
		{
            *value_ptr = tid->mExitCode;
		}
	}
    free(tid);
	return 0;
}

int pthread_detach(pthread_t tid)
{
	if (NULL == tid || tid->mDetached)
	{
		return EINVAL;
	}
	ptw32_mcs_local_node_t node;
	ptw32_mcs_lock_acquire(&ptw32_thread_reuse_lock, &node);
	if (0 == tid->mThreadID)
	{
		(void)WaitForSingleObject(tid->mThreadHandle, INFINITE);
		CloseHandle(tid->mThreadHandle);
		tid->mThreadHandle = NULL;
		free(tid);
	}
	else
	{
		tid->mDetached = true;
	}
	ptw32_mcs_lock_release(&node);
	return 0;
}

pthread_t pthread_self()
{
	return NULL;
}

//==============================================================================

int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr)
{
	pthread_mutex_t mx;
	int ret = 0;
	if (NULL == mutex)
	{
		return EINVAL;
	}
	mx = (pthread_mutex_t)calloc(1, sizeof(*mx));
	if (NULL == mx)
	{
		return ENOMEM;
	}
	InitializeCriticalSection(&(mx->mHandle));
	*mutex = mx;
	return 0;
}

int pthread_mutex_destroy(pthread_mutex_t* mutex)
{
	if (NULL == mutex || NULL == *mutex)
	{
		return EINVAL;
	}
	pthread_mutex_t mx = *mutex;
	DeleteCriticalSection(&(mx->mHandle));
	free(mx);
	*mutex = NULL;
	return 0;
}

int pthread_mutex_lock(pthread_mutex_t* mutex)
{
	int ret = 0;
	if (NULL == mutex || NULL == *mutex)
	{
		return EINVAL;
	}
	if (*mutex >= PTHREAD_ERRORCHECK_MUTEX_INITIALIZER)
	{
		ptw32_mcs_local_node_t node;
		ptw32_mcs_lock_acquire(&ptw32_mutex_test_init_lock, &node);
		if (*mutex >= PTHREAD_ERRORCHECK_MUTEX_INITIALIZER)
		{
			ret = pthread_mutex_init(mutex, NULL);
		}
		else if (NULL == *mutex)
		{
			ret = EINVAL;
		}		
		ptw32_mcs_lock_release(&node);
		if (0 != ret)
		{
			return ret;
		}
	}
	pthread_mutex_t mx = *mutex;
	EnterCriticalSection(&(mx->mHandle));
	return 0;
}

int pthread_mutex_unlock(pthread_mutex_t* mutex)
{
	if (NULL == mutex || NULL == *mutex)
	{
		return EINVAL;
	}
	if (PTHREAD_MUTEX_INITIALIZER == *mutex)
	{
		return 0;//ignore this unlock operation
	}
	pthread_mutex_t mx = *mutex;
	LeaveCriticalSection(&(mx->mHandle));
	return 0;
}

//==============================================================================

int pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t* attr)
{
	cond->mSemaphore = CreateSemaphoreW(NULL, 0, 0x7FFFFFFF, NULL);
	pthread_mutex_init(&cond->mLock, NULL);
	return 0;
}

int pthread_cond_destroy(pthread_cond_t* cond)
{
	pthread_mutex_destroy(&cond->mLock); 
	CloseHandle(cond->mSemaphore);
	return 0;
}

int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex)
{
	size_t wake = 0;
	size_t generation;
	pthread_mutex_lock(&cond->mLock);
	++cond->mWaiting;
	generation = cond->mGeneration;
	pthread_mutex_unlock(&cond->mLock);
	pthread_mutex_unlock(mutex);
	do {
		(void)WaitForSingleObject(cond->mSemaphore, INFINITE);
		pthread_mutex_lock(&cond->mLock);
		if (cond->mWake)
		{
			if (cond->mGeneration != generation)
			{
				--cond->mWake;
				--cond->mWaiting;
				break;
			}
			wake = 1;
		}
		pthread_mutex_unlock(&cond->mLock);
		if (wake)
		{
			wake = 0;
			ReleaseSemaphore(cond->mSemaphore, 1, NULL);
		}
	} while (1);
	pthread_mutex_unlock(&cond->mLock);
	pthread_mutex_lock(mutex);
	return 0;
}

int pthread_cond_signal(pthread_cond_t* cond)
{
	size_t wake = 0; 
	pthread_mutex_lock(&cond->mLock);
	if (cond->mWaiting > cond->mWake)
	{
		wake = 1;
		++cond->mWake;
		++cond->mGeneration;
	}
	pthread_mutex_unlock(&cond->mLock);
	if (wake)
	{
		ReleaseSemaphore(cond->mSemaphore, 1, NULL);
	}
	return 0;
}

int pthread_cond_broadcast(pthread_cond_t* cond)
{
	size_t wake = 0;
	pthread_mutex_lock(&cond->mLock);
	if (cond->mWaiting > cond->mWake)
	{
		wake = cond->mWaiting - cond->mWake;
		cond->mWake = cond->mWaiting;
		++cond->mGeneration;
	}
	pthread_mutex_unlock(&cond->mLock);
	if (wake)
	{
		ReleaseSemaphore(cond->mSemaphore, (LONG)wake, NULL);
	}
	return 0;
}

//==============================================================================

int pthread_rwlock_init(pthread_rwlock_t* rwlock, const pthread_rwlockattr_t* attr)
{
	pthread_rwlock_t rwl;
	if (NULL == rwlock || NULL == *rwlock)
	{
		return EINVAL;
	}
	rwl = (pthread_rwlock_t)calloc(1, sizeof(*rwl));
	if (NULL == rwl)
	{
		return ENOMEM;
	}
	rwl->m_mutex = CreateMutex(NULL, FALSE, NULL);
	rwl->m_readEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
	rwl->m_writeEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
	if (NULL == rwl->m_mutex || NULL == rwl->m_readEvent
		|| NULL == rwl->m_writeEvent)
	{
		pthread_rwlock_destroy(&rwl);
		return -2;
	}
	*rwlock = rwl;
	return 0;
}

int pthread_rwlock_destroy(pthread_rwlock_t* rwlock)
{
	pthread_rwlock_t rwl;
	if (NULL == rwlock || NULL == *rwlock)
	{
		return EINVAL;
	}
	if (PTHREAD_RWLOCK_INITIALIZER == *rwlock)
	{
		return 0;
	}
	rwl = *rwlock;
	if (rwl->m_mutex)
	{
		CloseHandle(rwl->m_mutex);
	}
	if (rwl->m_readEvent)
	{
		CloseHandle(rwl->m_readEvent);
	}
	if (rwl->m_writeEvent)
	{
		CloseHandle(rwl->m_writeEvent);
	}
	free(rwl);
	*rwlock = NULL;
	return 0;
}

static INLINE int ptw32_rwlock_check_need_init(pthread_rwlock_t* rwlock)
{
	int ret = 0;
	ptw32_mcs_local_node_t node;
	ptw32_mcs_lock_acquire(&ptw32_rwlock_test_init_lock, &node);
	if (PTHREAD_RWLOCK_INITIALIZER == *rwlock)
	{
		ret = pthread_rwlock_init(rwlock, NULL);
	}
	else if (NULL == *rwlock)
	{
		ret = EINVAL;
	}
	ptw32_mcs_lock_release(&node);
	return ret;
}

int pthread_rwlock_rdlock(pthread_rwlock_t* rwlock) 
{
	int ret;
	pthread_rwlock_t rwl;
	if (NULL == rwlock || NULL == *rwlock)
	{
		return EINVAL;
	}
	if (*rwlock == PTHREAD_RWLOCK_INITIALIZER)
	{
		if (0 != (ret = (ptw32_rwlock_check_need_init(rwlock))))
		{
			return ret;
		}
	}
	rwl = *rwlock;
	HANDLE h[2] = { rwl->m_mutex, rwl->m_readEvent };
	switch (WaitForMultipleObjects(2, h, TRUE, INFINITE))
	{
	case WAIT_OBJECT_0:
	case WAIT_OBJECT_0 + 1:
		++rwl->m_readers;
		ResetEvent(rwl->m_writeEvent);
		ReleaseMutex(rwl->m_mutex);
		assert(rwl->m_writers == 0);
		break;
	default:
		//cout << "cannot lock reader/writer lock" << endl;
		break;
	}
	return 0;
}

static inline void pri_pthread_rwlock_add_writer(pthread_rwlock_t* rwlock)
{
	pthread_rwlock_t rwl;
	if (NULL == rwlock || NULL == *rwlock)
	{
		return;
	}
	rwl = *rwlock;
	switch (WaitForSingleObject(rwl->m_mutex, INFINITE))
	{
	case WAIT_OBJECT_0:
		if (++rwl->m_writersWaiting == 1)
			ResetEvent(rwl->m_readEvent);
		ReleaseMutex(rwl->m_mutex);
		break;
	default:
		//cout << "cannot lock reader/writer lock" << endl;
		break;
	}
}

static inline void pri_pthread_rwlock_remove_writer(pthread_rwlock_t* rwlock)
{
	pthread_rwlock_t rwl;
	if (NULL == rwlock || NULL == *rwlock)
	{
		return;
	}
	rwl = *rwlock;
	switch (WaitForSingleObject(rwl->m_mutex, INFINITE))
	{
	case WAIT_OBJECT_0:
		if (--rwl->m_writersWaiting == 0 && rwl->m_writers == 0)
			SetEvent(rwl->m_readEvent);
		ReleaseMutex(rwl->m_mutex);
		break;
	default:
		//cout << "cannot lock reader/writer lock" << endl;
		break;
	}
}

int pthread_rwlock_wrlock(pthread_rwlock_t* rwlock)
{
	int ret;
	pthread_rwlock_t rwl;
	if (NULL == rwlock || NULL == *rwlock)
	{
		return EINVAL;
	}
	if (*rwlock == PTHREAD_RWLOCK_INITIALIZER)
	{
		if (0 != (ret = (ptw32_rwlock_check_need_init(rwlock))))
		{
			return ret;
		}
	}
	rwl = *rwlock;
	pri_pthread_rwlock_add_writer(rwlock);
	HANDLE h[2] = { rwl->m_mutex, rwl->m_writeEvent };
	switch (WaitForMultipleObjects(2, h, TRUE, INFINITE))
	{
	case WAIT_OBJECT_0:
	case WAIT_OBJECT_0 + 1:
		--rwl->m_writersWaiting;
		++rwl->m_readers;
		++rwl->m_writers;
		ResetEvent(rwl->m_readEvent);
		ResetEvent(rwl->m_writeEvent);
		ReleaseMutex(rwl->m_mutex);
		assert(rwl->m_writers == 1);
		break;
	default:
		pri_pthread_rwlock_remove_writer(rwlock);
		//cout << "cannot lock reader/writer lock" << endl;
		break;
	}
	return 0;
}

int pthread_rwlock_unlock(pthread_rwlock_t* rwlock)
{
	pthread_rwlock_t rwl;
	if (NULL == rwlock || NULL == *rwlock)
	{
		return EINVAL;
	}
	if (*rwlock == PTHREAD_RWLOCK_INITIALIZER)
	{
		return 0;
	}
	rwl = *rwlock;
	switch (WaitForSingleObject(rwl->m_mutex, INFINITE))
	{
	case WAIT_OBJECT_0:
		rwl->m_writers = 0;
		if (rwl->m_writersWaiting == 0) SetEvent(rwl->m_readEvent);
		if (--rwl->m_readers == 0) SetEvent(rwl->m_writeEvent);
		ReleaseMutex(rwl->m_mutex);
		break;
	default:
		//cout << "cannot unlock reader/writer lock" << endl;
		break;
	}
	return 0;
}

//==============================================================================

static DWORD WINAPI pri_win_thread_start_routine(LPVOID lpParam)
{
	void* exit_code;
	pthread_t pth = (pthread_t)(lpParam);

	exit_code = pth->mStart(pth->mArgs);
	pth->mExitCode = exit_code;

	ptw32_mcs_local_node_t node;
	ptw32_mcs_lock_acquire(&ptw32_thread_reuse_lock, &node);
	pth->mThreadID = 0;
	if (pth->mDetached)
	{
		CloseHandle(pth->mThreadHandle);
		free(pth);
	}
	ptw32_mcs_lock_release(&node);
	return 0;
}


#endif // _WIN32
