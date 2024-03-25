#if (1)
//#if(defined(HAVE_PTHREAD_H))

#include "thread/portable_thread.h" 
#undef GETTID
#undef THREAD_SETNAME_FOR_CURRENT
#include "thread/posix_thread.h"    /*  pthread/semaphore */
#include <malloc.h>
#include <string.h>

typedef struct
{
	pthread_t id;
} portable_thread_posix_t;

typedef struct
{
	pthread_mutex_t id;
} portable_mutex_posix_t;

typedef struct
{
	pthread_rwlock_t id;
} portable_rwlock_posix_t;

typedef struct
{
	sem_t id;
} portable_sem_posix_t;


unsigned long portable_gettid()
{	
	return (unsigned long)GETTID();
}

//==============================================================
int portable_thread_set_current_name(const char* name)
{
	return THREAD_SETNAME_FOR_CURRENT(name);
}

// create and start thread
int portable_thread_create(portable_thread_t* newthread_p,
	const portable_thread_attr_t* thread_attr,
	void* (*start_routine)(void*),
	void* arg)
{
	int ret;
	if (!newthread_p || !start_routine)
	{
		return -1;
	}
	*newthread_p = NULL;
	portable_thread_posix_t* posix = (portable_thread_posix_t*)malloc(sizeof(portable_thread_posix_t));
	if (NULL == posix)
	{
		return -2;
	}
	memset(posix, 0, sizeof(portable_thread_posix_t));
	ret = pthread_create(&posix->id, thread_attr ? thread_attr->platform_attr : NULL,
		start_routine, arg);
	if (0 == ret)
	{
		*newthread_p = posix;
		if (thread_attr && '\0' != thread_attr->name[0])
		{
			pthread_setname_np(posix->id, thread_attr->name);
		}
	}
	else
	{
		free(posix);
	}
	return ret;
}

// join thread until it exit
int portable_thread_join(portable_thread_t th, void** thread_return)
{
	int ret;
	portable_thread_posix_t* posix = (portable_thread_posix_t*)th;
	if (!posix /*|| 0 == posix->id*/)
	{
		return -1;
	}
	ret = pthread_join(posix->id, thread_return);
	//posix->id = 0;
	free(posix);
	return ret;
}
//=================================================================


//=================================================================
int portable_mutex_init(portable_mutex_t* mutex, void* mutex_attr)
{
	int ret;
	if (!mutex)
	{
		return -1;
	}
	*mutex = NULL;
	portable_mutex_posix_t* posix = (portable_mutex_posix_t*)malloc(sizeof(portable_mutex_posix_t));
	if (NULL == posix)
	{
		return -2;
	}
	memset(posix, 0, sizeof(portable_mutex_posix_t));
	ret = pthread_mutex_init(&posix->id, mutex_attr);
	if (0 == ret)
	{
		*mutex = posix;
	}
	else
	{
		free(posix);
	}
	return ret;
}

int portable_mutex_lock(portable_mutex_t* mutex)
{
	if (NULL == mutex || NULL == *mutex)
	{
		return -1;
	}
	portable_mutex_posix_t* posix = (portable_mutex_posix_t*)(*mutex);
	return pthread_mutex_lock(&posix->id);
}

int portable_mutex_unlock(portable_mutex_t* mutex)
{
	if (NULL == mutex)
	{
		return -1;
	}
	portable_mutex_posix_t* posix = (portable_mutex_posix_t*)(*mutex);
	return pthread_mutex_unlock(&posix->id);
}

int portable_mutex_destroy(portable_mutex_t* mutex)
{
	int ret;
	if (NULL == mutex || NULL == *mutex)
	{
		return -1;
	}

	portable_mutex_posix_t* posix = (portable_mutex_posix_t*)(*mutex);
	ret = pthread_mutex_destroy(&posix->id);
	free(posix);
	*mutex = NULL;
	return ret;
}
//=================================================================

//=================================================================
// init rwlock
int portable_rwlock_init(portable_rwlock_t* lock, const void* attr)
{
	int ret;
	if (!lock)
	{
		return -1;
	}
	*lock = NULL;
	portable_rwlock_posix_t* posix = (portable_rwlock_posix_t*)malloc(sizeof(portable_rwlock_posix_t));
	if (NULL == posix)
	{
		return -2;
	}
	memset(posix, 0, sizeof(portable_rwlock_posix_t));
	ret = pthread_rwlock_init(&posix->id, (const pthread_rwlockattr_t*)attr);
	if (0 == ret)
	{
		*lock = posix;
	}
	else
	{
		free(posix);
	}
	return ret;
}

// delete(destroy) rwlock
int portable_rwlock_destroy(portable_rwlock_t* lock)
{
	int ret;
	if (NULL == lock || NULL == *lock)
	{
		return -1;
	}

	portable_rwlock_posix_t* posix = (portable_rwlock_posix_t*)(*lock);
	ret = pthread_rwlock_destroy(&posix->id);
	free(posix);
	*lock = NULL;
	return ret;
}

// acquire read lock
int portable_rwlock_rdlock(portable_rwlock_t* lock)
{
	if (NULL == lock || NULL == *lock)
	{
		return -1;
	}
	portable_rwlock_posix_t* posix = (portable_rwlock_posix_t*)(*lock);
	return pthread_rwlock_rdlock(&posix->id);
}

// acquire write lock
int portable_rwlock_wrlock(portable_rwlock_t* lock)
{
	if (NULL == lock || NULL == *lock)
	{
		return -1;
	}
	portable_rwlock_posix_t* posix = (portable_rwlock_posix_t*)(*lock);
	return pthread_rwlock_wrlock(&posix->id);
}

// unlock read/write lock
int portable_rwlock_unlock(portable_rwlock_t* lock)
{
	if (NULL == lock || NULL == *lock)
	{
		return -1;
	}
	portable_rwlock_posix_t* posix = (portable_rwlock_posix_t*)(*lock);
	return pthread_rwlock_unlock(&posix->id);
}
//=================================================================


//=================================================================
// init sem
int portable_sem_init(portable_sem_t* sem, int pshared, unsigned int value)
{
	int ret;
	if (!sem)
	{
		return -1;
	}
	*sem = NULL;
	portable_sem_posix_t* posix = (portable_sem_posix_t*)malloc(sizeof(portable_sem_posix_t));
	if (NULL == posix)
	{
		return -2;
	}
	memset(posix, 0, sizeof(portable_sem_posix_t));
	ret = sem_init(&posix->id, pshared, value);
	if (0 == ret)
	{
		*sem = posix;
	}
	else
	{
		free(posix);
	}
	return ret;
}

// wait sem
int portable_sem_wait(portable_sem_t* sem)
{
	int ret;
	if (NULL == sem || NULL == *sem)
	{
		return -1;
	}
	portable_sem_posix_t* posix = (portable_sem_posix_t*)(*sem);
	ret = sem_wait(&posix->id);
	return ret;
}

// post sem
int portable_sem_post(portable_sem_t* sem)
{
	int ret;
	if (NULL == sem || NULL == *sem)
	{
		return -1;
	}
	portable_sem_posix_t* posix = (portable_sem_posix_t*)(*sem);
	ret = sem_post(&posix->id);
	return ret;
}

// get sem value
int portable_sem_getvalue(portable_sem_t* sem, int* sval)
{
	int ret;
	if (NULL == sem || NULL == *sem)
	{
		return -1;
	}
	portable_sem_posix_t* posix = (portable_sem_posix_t*)(*sem);
	ret = sem_getvalue(&posix->id, sval);
	return ret;
}

// destroy sem
int portable_sem_destroy(portable_sem_t* sem)
{
	int ret;
	if (NULL == sem || NULL == *sem)
	{
		return -1;
	}

	portable_sem_posix_t* posix = (portable_sem_posix_t*)(*sem);
	ret = sem_destroy(&posix->id);
	free(posix);
	*sem = NULL;
	return ret;
}
//=================================================================

#endif 
