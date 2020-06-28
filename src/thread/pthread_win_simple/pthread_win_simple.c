#include "pthread_win_simple.h"

#if(defined(_WIN32) && _LCU_CFG_WIN_PTHREAD_MODE == LCU_WIN_PTHREAD_IMPLEMENT_MODE_SIMPLE)
static DWORD WINAPI WinThreadStart(LPVOID lpParam);

int pthread_create(pthread_t* tid, const pthread_attr_t* attr, void* (*start)(void*), void* arg)
{
	tid->mStart = start;
	tid->mArgs = arg;
	tid->mThreadHandle = CreateThread(NULL, 0, WinThreadStart, (LPVOID)tid, CREATE_SUSPENDED, &tid->mThreadID);
	ResumeThread(tid->mThreadHandle);
	return 0;
}

int pthread_join(pthread_t tid, void** value_ptr)
{
	if (WaitForSingleObject(tid.mThreadHandle, INFINITE) != WAIT_FAILED)
	{
		CloseHandle(tid.mThreadHandle);
		if (value_ptr)
		{
			*value_ptr = tid.mExitCode;
		}
	}
	return 0;
}

int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr)
{
	InitializeCriticalSection(mutex);
	return 0;
}

int pthread_mutex_destroy(pthread_mutex_t* mutex)
{
	DeleteCriticalSection(mutex);
	return 0;
}

int pthread_mutex_lock(pthread_mutex_t* mutex)
{
	EnterCriticalSection(mutex);
	return 0;
}

int pthread_mutex_unlock(pthread_mutex_t* mutex)
{
	LeaveCriticalSection(mutex);
	return 0;
}

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
	DWORD ret;
	size_t wake = 0;
	size_t generation;
	pthread_mutex_lock(&cond->mLock);
	cond->mWaiting++;
	generation = cond->mGeneration;
	pthread_mutex_unlock(&cond->mLock);
	pthread_mutex_unlock(mutex);
	do {
		ret = WaitForSingleObject(cond->mSemaphore, INFINITE);
		pthread_mutex_lock(&cond->mLock);
		if (cond->mWake)
		{
			if (cond->mGeneration != generation)
			{
				cond->mWake--;
				cond->mWaiting--;
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
		cond->mWake++;
		cond->mGeneration++;
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
		cond->mGeneration++;
	}
	pthread_mutex_unlock(&cond->mLock);
	if (wake)
	{
		ReleaseSemaphore(cond->mSemaphore, (LONG)wake, NULL);
	}
	return 0;
}

static DWORD WINAPI WinThreadStart(LPVOID lpParam)
{
	void* exitcode;
	pthread_t* pth = (pthread_t*)(lpParam);
	exitcode = pth->mStart(pth->mArgs);
	pth->mExitCode = exitcode;
	pth->mThreadID = 0;
	return 0;
}

#endif 