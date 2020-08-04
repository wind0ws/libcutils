#include "mem/mplite.h"
#include <stdio.h>
#include <stdlib.h>
#include "thread/thread_wrapper.h"

typedef struct multithreaded_param 
{
	size_t index;
	mplite_t* pool;
	int req_size;
} multithreaded_param_t;

static void* multithreaded_main(void* args)
{
	multithreaded_param_t* param;
	void* buffer;
	int round_size;

	param = (multithreaded_param_t*)args;

	round_size = mplite_roundup(param->pool, param->req_size);
	printf("index: %zu requested size: %d round-up size: %d\n", param->index,
		param->req_size, round_size);
	while ((buffer = mplite_malloc(param->pool, param->req_size)) != NULL) 
	{
		printf("index: %zu address: %p size: %d\n", param->index, buffer,
			param->req_size);
		/* Sleep for 1 millisecond to give other threads to run */
		usleep(1000);
	}
	return NULL;
}

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4996) //for disable scanf warning
#endif // _WIN32
int mplite_test()
{
	size_t alloc_counter;
	size_t buffer_size;
	size_t min_alloc;
	size_t num_threads;
	char* large_buffer;
	char* alloc_ret;
	mplite_t mempool;
	pthread_t* threads;
	multithreaded_param_t* threads_param;
	mplite_lock_t pool_lock;
	pthread_mutex_t mutex;
	int test_again;
	int scanf_ret;

	pthread_mutex_init(&mutex, NULL);

	/* Get the buffer and size */
	do {
		printf("Memory pool testing using mplite API\n");

		printf("Enter the total memory block size: ");
		scanf_ret = scanf("%zu", &buffer_size);
		if (scanf_ret!=1)
		{
			break;
		}
		printf("Enter the minimum memory allocation size: ");
		scanf_ret = scanf("%zu", &min_alloc);
		if (scanf_ret != 1)
		{
			break;
		}
		printf("Enter the number of threads to run for multi-threaded test: ");
		scanf_ret = scanf("%zu", &num_threads);
		if (scanf_ret != 1)
		{
			break;
		}

		large_buffer = malloc(buffer_size);
		if (!large_buffer)
		{
			break;
		}
		printf("buffer = %p size = %zu minimum alloc = %zu\n", 
			large_buffer, buffer_size, min_alloc);

		mplite_init(&mempool, large_buffer, (const int)buffer_size,
			(const int)min_alloc, NULL);
		printf("\nSingle-threaded test...\n");
		alloc_counter = 1;
		while ((alloc_ret = mplite_malloc(&mempool, (const int)min_alloc)) != NULL) 
		{
			printf("malloc = %p counter = %zu\n", alloc_ret, alloc_counter);
			alloc_counter++;
		}
		mplite_print_stats(&mempool, puts);

		printf("\nMulti-threaded test...\n");
		pool_lock.arg = (void*)&mutex;
		pool_lock.acquire = (int (*)(void*))pthread_mutex_lock;
		pool_lock.release = (int (*)(void*))pthread_mutex_unlock;
		mplite_init(&mempool, large_buffer, (const int)buffer_size,
			(const int)min_alloc, &pool_lock);
		threads = (pthread_t*)malloc(sizeof(*threads) * num_threads);
		threads_param = (multithreaded_param_t*)malloc(sizeof(*threads_param) * num_threads);
		/* Run all the threads */
		for (alloc_counter = 0; alloc_counter < num_threads; alloc_counter++) 
		{
			threads_param[alloc_counter].index = alloc_counter;
			threads_param[alloc_counter].req_size = (const int)min_alloc;
			threads_param[alloc_counter].pool = &mempool;
			pthread_create(&threads[alloc_counter], NULL, multithreaded_main,
				&threads_param[alloc_counter]);
		}
		/* Wait for all the threads to finish */
		for (alloc_counter = 0; alloc_counter < num_threads; alloc_counter++) 
		{
			pthread_join(threads[alloc_counter], NULL);
		}
		mplite_print_stats(&mempool, puts);
		free(threads_param);
		free(threads);

		free(large_buffer);

		printf("Test again? <0:false, non-zero:true>: ");
		scanf_ret = scanf("%d", &test_again);
		if (scanf_ret != 1)
		{
			break;
		}
	} while (test_again);

	pthread_mutex_destroy(&mutex);

	return 0;
}
#ifdef _WIN32
#pragma warning(pop)  
#endif // _WIN32